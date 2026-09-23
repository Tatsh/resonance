#include "os/mem.h"

#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "os/log.h"
#include "os/zone.h"

// The linker script defines these three, and each carries its value in its address: the base of
// the stack, its size, and the end of the loaded image.
extern "C" char _stack[];
extern "C" char _stack_size[];
extern "C" char _end[];

// Five of the log lines below pass a size_t through %d, which the format literals in the image do.
// size_t is 32 bits on the Emotion Engine, where the pairing is exact, and the cross build reports
// nothing. A 64-bit host widens the argument and warns. Neither the literal nor the argument is
// changed for that: the literal is the one the image stores, and the warning describes the host.

namespace {

// The interned source table MemLogFindSource() matches against.
constexpr int kMemLogSourceCount = 128;

// A source name of this length or more is rejected, which is the size of the name field.
constexpr int kMemLogSourceNameLimit = 40;

// One row of the per-source report MemLogSourceReport() prints, 0x40 bytes. The counters are
// titled from the report's column heading.
struct MemLogSource {
    char mName[kMemLogSourceNameLimit]; // +0x00
    int mAllocCount;                    // +0x28 totalloc
    int mLiveCount;                     // +0x2c curalloc
    int mPeakCount;                     // +0x30 hialloc
    int mTotalBytes;                    // +0x34 totbytes
    int mLiveBytes;                     // +0x38 curbytes
    int mPeakBytes;                     // +0x3c hibytes
};

// One tracked block of the table MemLogSourceInit() allocates, 0x10 bytes. A block hashes to the
// slot its address selects, and a collision takes the next free slot after the chain's tail.
struct MemLogBlock {
    void *mBlock;       // +0x00 null for a free slot
    int mSize;          // +0x04
    int mSource;        // +0x08 row of g_aMemLogSources
    MemLogBlock *mNext; // +0x0c
};

// Slots of the block table, which the address hash is masked to.
constexpr int kMemLogBlockCount = 0x200000;
constexpr int kMemLogBlockMask = kMemLogBlockCount - 1;

// The low address bits the block hash discards.
constexpr int kMemLogBlockHashShift = 4;

// Byte the stack is painted with, so that the deepest write can be found afterwards.
constexpr int kStackPaintByte = 'u';

// Bytes at the top of the stack the paint leaves alone, because the painting routine is running
// there.
constexpr int kStackPaintReserve = 0x2000;

// Largest line MemLogCloseAndContinue() copies from the old report at once.
constexpr int kMemLogLineSize = 0x800;

// Room for the extension MemLogCloseAndContinue() moves past the reopen count.
constexpr int kMemLogExtensionSize = 0x30;

// Bytes of each report path, bounded by the next global rather than measured.
constexpr int kMemLogPathSize = 0x40;

// Room for one line of the report MemEndAccounting() builds, and the margin it keeps free.
constexpr int kAccountingLineSize = 0x80;
constexpr unsigned kAccountingReportMargin = 0x40;

// The tag the untagged array allocation path bills to.
constexpr char kUntaggedTag[] = "UNK[]";

// The tag the untagged single-object allocation path bills to.
constexpr char kScalarTag[] = "UNK";

// The tag the STL allocator hook is rewound to after every tagged allocation.
constexpr char kStlUnknownTag[] = "stl_unk";

// The block size ReportHeapCapacity() probes with.
constexpr int kHeapProbeBlockSize = 0x800;

// DumpHeapMemoryLog() probes the largest single allocation downward from 1280 steps of a tenth of
// a megabyte, and reports the step count in megabytes.
constexpr unsigned kLargestProbeSteps = 1280;
constexpr unsigned kLargestProbeStepSize = 102400;
constexpr double kLargestProbeStepsPerMegabyte = 10.0;

// DumpHeapMemoryLog() then counts how many blocks of each size fit, up to this many, and reports
// each block as costing its size plus the allocator's overhead.
constexpr int kProbeBlockLimit = 0x10000;
constexpr int kProbeBlockOverhead = 16;

// Bytes of the path DumpHeapMemoryLog() formats each file name into.
constexpr int kHeapLogPathSize = 0x40;

// The blocks one counting pass of DumpHeapMemoryLog() holds before releasing them.
// 0x0089e200
void *g_apProbeBlocks[kProbeBlockLimit];

// 0x006f57d0
int g_bMemLogging;

// The rewind to kStlUnknownTag compiles to one doubleword store. The array bound comes from the
// distance to the next global rather than from any single access.
// 0x006f57d8
char g_szStlAllocTag[kMemStlTagSize];

// 0x006f5858
int g_bMemAccounting;

// 0x006f585c
int g_nMemTotalBytes;

// 0x006f5860
MemTagTotal g_aMemTagTotals[kMemTagCount];

// 0x006f6460
MemLogSource g_aMemLogSources[kMemLogSourceCount];

// Null until MemLogSourceInit() runs, which disables the tracking routines.
// 0x006f8460
MemLogBlock *g_pMemLogBlocks;

// Set once MemOpenLog() has painted the stack.
// 0x006f57c8
int g_bMemStackPainted;

// Reports MemLogCloseAndContinue() has started since MemOpenLog().
// 0x006f57cc
int g_nMemLogReopenCount;

// 0x00894d70
FILE *g_pMemLogFile;

// Path of the report being written.
// 0x00894db8
char g_szMemLogPath[kMemLogPathSize];

// Path MemOpenLog() was given, which each reopened report is named after.
// 0x00894d78
char g_szMemLogBaseName[kMemLogPathSize];

// 0x004a9360
// The address is an out-of-line copy with no caller. Reduces a tag to the text after its last path
// separator. Both separators are tried, and a tag recorded on a Windows build host still logs as a
// basename.
inline const char *TagBasename(const char *pszTag) {
    const char *pName = strrchr(pszTag, '/');
    pName = (pName == nullptr) ? pszTag : pName + 1;
    const char *pAfterBackslash = strrchr(pName, '\\');
    return (pAfterBackslash == nullptr) ? pName : pAfterBackslash + 1;
}

// Bills a request to the accounting table. Both allocation paths inline this
// block. An unseen tag claims the first record whose name is still empty, and a
// full table charges the overflow record.
inline void ChargeTagTotal(const char *pszTag, size_t nSize) {
    (void)strlen(pszTag); // Yes, the binary discards this call's result.
    g_nMemTotalBytes += nSize;
    for (int i = 1; i < kMemTagCount; ++i) {
        MemTagTotal *pRecord = &g_aMemTagTotals[i];
        if (pRecord->mName[0] == '\0') {
            strcpy(pRecord->mName, pszTag);
        }
        if (strcmp(pszTag, pRecord->mName) == 0) {
            pRecord->mBytes += nSize;
            return;
        }
    }
    g_aMemTagTotals[0].mBytes += nSize;
}

// The value of a linker-script symbol, which carries its meaning in its address.
inline unsigned LinkerAddress(const char *pSymbol) {
    return static_cast<unsigned>(reinterpret_cast<uintptr_t>(pSymbol));
}

// The slot of the block table an address hashes to.
inline int BlockSlot(const void *pBlock) {
    return (static_cast<int>(reinterpret_cast<intptr_t>(pBlock)) >> kMemLogBlockHashShift) &
           kMemLogBlockMask;
}

// The tracked entry for a block, or null. The walk stops at the first free slot of the chain.
inline MemLogBlock *FindTrackedBlock(const void *pBlock) {
    for (MemLogBlock *pEntry = &g_pMemLogBlocks[BlockSlot(pBlock)]; pEntry != nullptr;
         pEntry = pEntry->mNext) {
        if (pEntry->mBlock == nullptr) {
            return nullptr;
        }
        if (pEntry->mBlock == pBlock) {
            return pEntry;
        }
    }
    return nullptr;
}

// 0x004a9620
// an out-of-line copy with no caller; MemLogSourceTrackRealloc() expands it. Records
// a block against its source. The new entry is linked only when the chain it joins has a second
// entry, which is what the binary does.
inline void TrackBlock(const char *pszSource, void *pBlock, int nSize) {
    if (g_pMemLogBlocks == nullptr) {
        return;
    }
    MemLogBlock *pEntry = &g_pMemLogBlocks[BlockSlot(pBlock)];
    MemLogBlock *pTail = nullptr;
    while (pEntry->mNext != nullptr) {
        pEntry = pEntry->mNext;
        pTail = pEntry;
    }
    while (pEntry->mBlock != nullptr) {
        pEntry = &g_pMemLogBlocks[(pEntry - g_pMemLogBlocks + 1) & kMemLogBlockMask];
    }
    if (pTail != nullptr) {
        pTail->mNext = pEntry;
    }
    const int nSource = MemLogFindSource(pszSource);
    pEntry->mBlock = pBlock;
    pEntry->mNext = nullptr;
    pEntry->mSize = nSize;
    pEntry->mSource = nSource;
    MemLogSource &source = g_aMemLogSources[nSource];
    ++source.mAllocCount;
    ++source.mLiveCount;
    if (source.mPeakCount < source.mLiveCount) {
        source.mPeakCount = source.mLiveCount;
    }
    source.mTotalBytes += nSize;
    source.mLiveBytes += nSize;
    if (source.mPeakBytes < source.mLiveBytes) {
        source.mPeakBytes = source.mLiveBytes;
    }
}

// The C library's heap statistics. A current glibc marks mallinfo() deprecated, which the
// Emotion Engine's newlib does not; the pragma keeps a host build free of that warning only.
inline struct mallinfo ReadMallinfo() {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    return mallinfo();
#pragma GCC diagnostic pop
}

// The ten mallinfo lines MemCloseLogAndReport() and MemLogCloseAndContinue() share.
inline void LogMallinfo(const struct mallinfo &info) {
    LogPrintf("   arena:    %d   (total space allocated from system)\n", info.arena);
    LogPrintf("   ordblks:  %d   (number of non-inuse chunks)\n", info.ordblks);
    LogPrintf("   smblks:   %d   (unused)\n", info.smblks);
    LogPrintf("   hblks:    %d   (number of mmapped regions)\n", info.hblks);
    LogPrintf("   hblkhd:   %d   (total space in mmapped regions)\n", info.hblkhd);
    LogPrintf("   usmblks:  %d   (unused)\n", info.usmblks);
    LogPrintf("   fsmblks:  %d   (unused)\n", info.fsmblks);
    LogPrintf("   uordblks: %d   (total allocated space)\n", info.uordblks);
    LogPrintf("   fordblks: %d   (total non-inuse space)\n", info.fordblks);
    LogPrintf("   keepcost: %d   (top-most, releaseable (via malloc_trim) space)\n", info.keepcost);
}

// The stack depth both report routines log once MemOpenLog() has painted the stack. The deepest
// write is the first byte from the bottom that no longer holds the paint.
inline void LogStackUse() {
    if (g_bMemStackPainted == 0) {
        return;
    }
    const int nStackSize = static_cast<int>(LinkerAddress(_stack_size));
    const int nPainted = nStackSize - kStackPaintReserve;
    int nUntouched = 0;
    while (nUntouched < nPainted && _stack[nUntouched] == kStackPaintByte) {
        ++nUntouched;
    }
    LogPrintf(
        "STACK AREA USED IS %d BYTES of STACK SIZE %d\n", nStackSize - nUntouched, nStackSize);
}

} // namespace

// 0x004a8380
void *operator new[](size_t nSize) {
    size_t nRequest = (nSize != 0) ? nSize : 1;
    void *pBlock = HeapAlloc(nRequest);

    if (g_bMemAccounting != 0) {
        ChargeTagTotal(kUntaggedTag, nRequest);
    }
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "new(UNK[],%d,%p)\n", nRequest, pBlock);
    }
    if (pBlock == nullptr) {
        Fatal("NEW[] ALLOCATION FAILURE, size: %d\n", nRequest);
    }
    return pBlock;
}

// 0x004a8520
void *MemAllocTagged(size_t nSize, const char *pszTag, int nLine) {
    void *pBlock = HeapAlloc(nSize);

    if (g_bMemAccounting != 0) {
        ChargeTagTotal(pszTag, nSize);
    }
    if (g_bMemLogging != 0) {
        fprintf(
            g_pMemLogFile, "malloc(%s_%d,%d,0x%p)\n", TagBasename(pszTag), nLine, nSize, pBlock);
        strcpy(g_szStlAllocTag, kStlUnknownTag);
    }
    if (pBlock == nullptr) {
        Fatal("MEMORY ALLOCATION FAILURE, file: %s, line: %d, size: %d\n", pszTag, nLine, nSize);
    }
    return pBlock;
}

// 0x004a81e0
void *operator new(size_t nSize) {
    size_t nRequest = (nSize != 0) ? nSize : 1;
    void *pBlock = HeapAlloc(nRequest);

    if (g_bMemAccounting != 0) {
        ChargeTagTotal(kScalarTag, nRequest);
    }
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "new(UNK,%d,%p)\n", nRequest, pBlock);
    }
    if (pBlock == nullptr) {
        Fatal("NEW ALLOCATION FAILURE, size: %d\n", nRequest);
    }
    return pBlock;
}

// 0x004a90a0
void *AllocateTaggedMemory(size_t nSize, const char *pszClass) {
    size_t nRequest = (nSize != 0) ? nSize : 1;
    void *pBlock = HeapAlloc(nRequest);

    if (g_bMemAccounting != 0) {
        ChargeTagTotal(pszClass, nRequest);
    }
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "new(%s,%d,%p)\n", pszClass, nRequest, pBlock);
    }
    if (pBlock == nullptr) {
        Fatal("NEW ALLOCATION FAILURE, class: %s, size: %d\n", pszClass, nRequest);
    }
    return pBlock;
}

// 0x004a92c8
// The image, built as C++98, has no sized overload to pair with this one.
void operator delete[](void *pBlock) noexcept {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "del(UNK[],%p)\n", pBlock);
    }
    HeapFree(pBlock);
}

// 0x004a9230
// The image, built as C++98, has no sized overload to pair with this one.
void operator delete(void *pBlock) noexcept {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "del(UNK,%p)\n", pBlock);
    }
    HeapFree(pBlock);
}

// 0x004a91e0
void FreeTaggedMemory(void *pBlock, const char *pszClass) {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "del(%s,%p)\n", pszClass, pBlock);
    }
    HeapFree(pBlock);
}

// 0x004a94e8
void MemFreeTagged(void *pBlock, const char *pszTag, int nLine) {
    int nZone = FindZoneForPointer(pBlock);
    if (nZone != kNoZone) {
        Fatal("ZONE FREE ERROR: ptr %p in zone %d, can't free()\n", pBlock, nZone);
    }

    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "free(%s_%d,0x%p)\n", TagBasename(pszTag), nLine, pBlock);
    }
    HeapFree(pBlock);
}

// 0x004a93b8
void *MemReallocTagged(void *pBlock, size_t nSize, const char *pszTag, int nLine) {
    int nZone = FindZoneForPointer(pBlock);
    if (nZone != kNoZone) {
        Fatal("ZONE REALLOC ERROR: ptr %p in zone %d, can't realloc()\n", pBlock, nZone);
    }

    void *pNew = HeapRealloc(pBlock, nSize);
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile,
                "realloc(%s_%d,%d,0x%p,0x%p)\n",
                TagBasename(pszTag),
                nLine,
                nSize,
                pBlock,
                pNew);
        strcpy(g_szStlAllocTag, kStlUnknownTag);
    }
    if (pNew == nullptr) {
        Fatal("MEMORY RE-ALLOCATION FAILURE, file: %s, line: %d, size: %d\n", pszTag, nLine, nSize);
    }
    return pNew;
}

// 0x004a9090
char *MemGetCurrentTag() {
    return g_szStlAllocTag;
}

// 0x004a9048
void MemSetStlTag(const char *pszKind, int nElemSize) {
    sprintf(g_szStlAllocTag, "%s.%d", pszKind, nElemSize);
}

// 0x004a8e18
void MemLogWrite(const char *pszText) {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "MARKER: %s\n", pszText);
    }
}

// 0x004a86c0
int MemLogFindSource(const char *pszName) {
    const char *pName = TagBasename(pszName);

    int nRow = 0;
    if (g_aMemLogSources[0].mName[0] != '\0') {
        for (;;) {
            if (strcmp(g_aMemLogSources[nRow].mName, pName) == 0) {
                return nRow;
            }
            ++nRow;
            if (nRow >= kMemLogSourceCount) {
                break;
            }
            if (g_aMemLogSources[nRow].mName[0] == '\0') {
                break;
            }
        }
    }

    if (nRow == kMemLogSourceCount) {
        Fatal("MemLogFindSource: source table full!\n");
    }
    if (strlen(pName) >= kMemLogSourceNameLimit) {
        Fatal("MemLogFindSource: name %s too long\n", pName);
    }
    strcpy(g_aMemLogSources[nRow].mName, pName);
    return nRow;
}

// 0x004a95c8
void MemLogSourceInit() {
    g_pMemLogBlocks =
        static_cast<MemLogBlock *>(HeapAlloc(kMemLogBlockCount * sizeof(MemLogBlock)));
    if (g_pMemLogBlocks != nullptr) {
        memset(g_pMemLogBlocks, 0, kMemLogBlockCount * sizeof(MemLogBlock));
        memset(g_aMemLogSources, 0, sizeof(g_aMemLogSources));
    }
}

// 0x004a87d0
void MemLogSourceTrackRealloc(const char *pszSource, void *pNew, void *pOld, int nSize) {
    if (g_pMemLogBlocks == nullptr) {
        return;
    }
    MemLogBlock *pEntry = FindTrackedBlock(pOld);
    if (pEntry == nullptr) {
        LogPrintf("MemLogSourceTrackRealloc(): can't find realloc for src: %s\n", pszSource);
        TrackBlock(pszSource, pNew, nSize);
        return;
    }
    // Yes, the binary retains the entry in the old block's chain under the new address.
    const int nDelta = nSize - pEntry->mSize;
    pEntry->mBlock = pNew;
    pEntry->mSize = nSize;
    MemLogSource &source = g_aMemLogSources[pEntry->mSource];
    source.mTotalBytes += nDelta;
    source.mLiveBytes += nDelta;
    if (source.mPeakBytes < source.mLiveBytes) {
        source.mPeakBytes = source.mLiveBytes;
    }
}

// 0x004a8a68
void MemLogSourceReport(const char *pszTitle, FILE *pFile) {
    if (g_pMemLogBlocks == nullptr) {
        return;
    }
    MemLogSource aSources[kMemLogSourceCount];
    int nCount = 0;
    while (nCount < kMemLogSourceCount) {
        aSources[nCount] = g_aMemLogSources[nCount];
        if (aSources[nCount].mName[0] == '\0') {
            break;
        }
        ++nCount;
    }
    for (int i = 0; i < nCount - 1; ++i) {
        for (int j = i + 1; j < nCount; ++j) {
            if (strcmp(aSources[i].mName, aSources[j].mName) > 0) {
                const MemLogSource swap = aSources[i];
                aSources[i] = aSources[j];
                aSources[j] = swap;
            }
        }
    }

    if (pFile == nullptr) {
        pFile = stdout;
    }
    fprintf(pFile, "%s\n", pszTitle);
    fprintf(pFile,
            "                  *** NAME ***  totalloc curalloc  hialloc  totbytes curbytes  "
            "hibytes\n");
    fprintf(pFile,
            "------------------------------  -------- -------- --------  -------- -------- "
            "---------\n");
    for (int i = 0; i < nCount; ++i) {
        const MemLogSource &source = aSources[i];
        fprintf(pFile,
                "%30s  %8d %8d %8d  %8d %8d %8d\n",
                source.mName,
                source.mAllocCount,
                source.mLiveCount,
                source.mPeakCount,
                source.mTotalBytes,
                source.mLiveBytes,
                source.mPeakBytes);
    }
}

// 0x004a8e50
void MemLogPrint(const char *pszText) {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "%s", pszText);
    }
}

// 0x004a8e88
void MemBeginAccounting() {
    g_nMemTotalBytes = 0;
    memset(g_aMemTagTotals, 0, sizeof(g_aMemTagTotals));
    strcpy(g_aMemTagTotals[0].mName, "Other_Sources");
    g_bMemAccounting = 1;
}

// 0x004a8ef8
int MemEndAccounting(char *pszReport, int nReportSize) {
    sprintf(pszReport, "Memory Allocated: %d\n", g_nMemTotalBytes);
    for (int i = 0; i < kMemTagCount; ++i) {
        const MemTagTotal &record = g_aMemTagTotals[i];
        if (record.mName[0] == '\0') {
            continue;
        }
        if (i == 0 && record.mBytes <= 0) {
            continue;
        }
        char szLine[kAccountingLineSize];
        // Yes, the binary's line buffer is shorter than the longest tag the table admits.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-overflow"
        sprintf(szLine, "   %s: alloced: %d\n", record.mName, record.mBytes);
#pragma GCC diagnostic pop
        if (static_cast<unsigned>(nReportSize) - kAccountingReportMargin <
            strlen(pszReport) + strlen(szLine)) {
            sprintf(&pszReport[strlen(pszReport)], "...REPORT TOO LONG FOR BUFFER!\n");
            break;
        }
        strcat(pszReport, szLine);
    }
    g_bMemAccounting = 0;
    return g_nMemTotalBytes;
}

// 0x004a7c40
void MemOpenLog(const char *pszPath) {
    if (pszPath != nullptr) {
        strcpy(g_szMemLogPath, pszPath);
        strcpy(g_szMemLogBaseName, pszPath);
        g_nMemLogReopenCount = 0;
        g_pMemLogFile = fopen(g_szMemLogPath, "w");
        if (g_pMemLogFile != nullptr) {
            g_bMemLogging = 1;
        }
    }
    LogPrintf("_stack = $%x\n", LinkerAddress(_stack));
    LogPrintf("_stack_size = $%x\n", LinkerAddress(_stack_size));
    LogPrintf("_end = $%x\n", LinkerAddress(_end));
    if (static_cast<int>(LinkerAddress(_stack)) > 0) {
        g_bMemStackPainted = 1;
        memset(_stack, kStackPaintByte, LinkerAddress(_stack_size) - kStackPaintReserve);
    }
    atexit(MemCloseLogAndReport);
}

// 0x004a7d30
void MemCloseLogAndReport() {
    if (g_pMemLogFile != nullptr) {
        fclose(g_pMemLogFile);
        g_pMemLogFile = nullptr;
        g_bMemLogging = 0;
    }
    const struct mallinfo info = ReadMallinfo();
    LogPrintf("system heap info (mallinfo):\n");
    LogMallinfo(info);
    LogStackUse();
    DumpHeapMemoryLog(0);
}

// 0x004a7ef8
void MemLogCloseAndContinue() {
    LogPrintf("MemLogCloseAndContinue:, fpLog: %p\n", g_pMemLogFile);
    if (g_pMemLogFile != nullptr) {
        fclose(g_pMemLogFile);
        FILE *pOld = fopen(g_szMemLogPath, "r");
        ++g_nMemLogReopenCount;
        strcpy(g_szMemLogPath, g_szMemLogBaseName);
        char *pszExtension = strchr(g_szMemLogPath, '.');
        if (pszExtension == nullptr) {
            pszExtension = &g_szMemLogPath[strlen(g_szMemLogPath)];
        }
        char szExtension[kMemLogExtensionSize];
        strcpy(szExtension, pszExtension);
        sprintf(pszExtension, "_%d%s", g_nMemLogReopenCount, szExtension);
        g_pMemLogFile = fopen(g_szMemLogPath, "w");
        LogPrintf("reopened %s at %p\n", g_szMemLogPath, g_pMemLogFile);
        char szLine[kMemLogLineSize];
        while (fgets(szLine, kMemLogLineSize, pOld) != nullptr) {
            fputs(szLine, g_pMemLogFile);
        }
        fclose(pOld);
    }
    const struct mallinfo info = ReadMallinfo();
    LogPrintf("system heap info (mallinfo) at dump %d:\n", g_nMemLogReopenCount);
    LogMallinfo(info);
    LogStackUse();
}

// HeapAlloc(), HeapFree(), and HeapRealloc() have no bodies here. Each is two instructions that
// load the toolchain allocator's state from 0x007819cc and tail-call it, and the allocator itself
// is toolchain code that this tree does not reconstruct.

// 0x00246c18
void ReportHeapCapacity() {
    std::vector<void *> blocks;
    int nBlocks = 0;
    void *pBlock;
    while ((pBlock = HeapAlloc(kHeapProbeBlockSize)) != nullptr) {
        blocks.push_back(pBlock);
        ++nBlocks;
    }
    LogPrintf("Able to allocate %d blocks of size %d (%d bytes total)\n",
              nBlocks,
              kHeapProbeBlockSize,
              nBlocks * kHeapProbeBlockSize);
    for (void *pAllocated : blocks) {
        HeapFree(pAllocated);
    }
}

// 0x0054b348
void DumpHeapMemoryLog(int nIndex) {
    char szPath[kHeapLogPathSize];
    sprintf(szPath, "memdump_%d.txt", nIndex);
    FILE *pFile = fopen(szPath, "w");
    MemLogSourceReport("MEMLOG STATS", pFile);
    fclose(pFile);

    sprintf(szPath, "memstat_%d.txt", nIndex);
    pFile = fopen(szPath, "w");

    void *pLargest = nullptr;
    unsigned nSteps = kLargestProbeSteps;
    while (nSteps != 0) {
        pLargest = HeapAlloc(nSteps * kLargestProbeStepSize);
        if (pLargest != nullptr) {
            break;
        }
        --nSteps;
    }
    if (pLargest != nullptr) {
        LogPrintf("Largest possible allocation: %f megabytes\n",
                  static_cast<float>(nSteps) / kLargestProbeStepsPerMegabyte);
        fprintf(pFile,
                "Largest possible allocation: %f megabytes\n",
                static_cast<float>(nSteps) / kLargestProbeStepsPerMegabyte);
        HeapFree(pLargest);
    }

    const int anBlockSizes[] = {2048, 128};
    for (const int nBlockSize : anBlockSizes) {
        int nCount = 0;
        while (nCount < kProbeBlockLimit) {
            g_apProbeBlocks[nCount] = HeapAlloc(nBlockSize);
            if (g_apProbeBlocks[nCount] == nullptr) {
                break;
            }
            ++nCount;
        }
        LogPrintf("Able to allocate %d blocks of size %d (%d bytes total)\n",
                  nCount,
                  nBlockSize,
                  nCount * (nBlockSize + kProbeBlockOverhead));
        fprintf(pFile,
                "Able to allocate %d blocks of size %d (%d bytes total)\n",
                nCount,
                nBlockSize,
                nCount * (nBlockSize + kProbeBlockOverhead));
        for (int i = 0; i < nCount; ++i) {
            HeapFree(g_apProbeBlocks[i]);
        }
    }

    fclose(pFile);
}
