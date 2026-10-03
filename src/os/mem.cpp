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
// named after the report's column heading.
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
// NTSC-U/C: 0x0089e200, PAL: 0x008e31c0
void *g_apProbeBlocks[kProbeBlockLimit] = {};

// NTSC-U/C: 0x006f57d0, PAL: 0x00739220
int bEnabled = 0;

// The rewind to kStlUnknownTag compiles to one doubleword store. The array bound comes from the
// distance to the next global rather than from any single access.
// NTSC-U/C: 0x006f57d8, PAL: 0x00739228
char stlName[kMemStlTagSize] = "stl_unk";

// NTSC-U/C: 0x006f5858, PAL: 0x007392a8
int bMemCountEnabled = 0;

// NTSC-U/C: 0x006f585c, PAL: 0x007392ac
int memCountAlloced = 0;

// NTSC-U/C: 0x006f5860, PAL: 0x007392b0
MemTagTotal memCountSource[kMemTagCount];

// NTSC-U/C: 0x006f6460, PAL: 0x00739eb0
MemLogSource g_aMemLogSources[kMemLogSourceCount];

// Null until MemLogSourceInit() runs, which disables the tracking routines.
// NTSC-U/C: 0x006f8460, PAL: 0x0073beb0
MemLogBlock *g_pMemLogBlocks = nullptr;

// Set once MemOpenLog() has painted the stack.
// NTSC-U/C: 0x006f57c8, PAL: 0x00739218
int g_bMemStackPainted = 0;

// Reports MemLogCloseAndContinue() has started since MemOpenLog().
// NTSC-U/C: 0x006f57cc, PAL: 0x0073921c
int g_nMemLogReopenCount = 0;

// NTSC-U/C: 0x00894d70, PAL: 0x008d9d80
FILE *fpLog = nullptr;

// Path of the report being written.
// NTSC-U/C: 0x00894db8, PAL: 0x008d9dc8
char g_szMemLogPath[kMemLogPathSize] = {};

// Path MemOpenLog() was given, which each reopened report is named after.
// NTSC-U/C: 0x00894d78, PAL: 0x008d9d88
char g_szMemLogBaseName[kMemLogPathSize] = {};

// NTSC-U/C: 0x004a9360, PAL: 0x004e7470
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
    memCountAlloced += nSize;
    for (int i = 1; i < kMemTagCount; ++i) {
        MemTagTotal *pRecord = &memCountSource[i];
        if (pRecord->mName[0] == '\0') {
            strcpy(pRecord->mName, pszTag);
        }
        if (strcmp(pszTag, pRecord->mName) == 0) {
            pRecord->mBytes += nSize;
            return;
        }
    }
    memCountSource[0].mBytes += nSize;
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

// NTSC-U/C: 0x004a9620, PAL: 0x004e7730
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

// NTSC-U/C: 0x004a8380, PAL: 0x004e6490
void *operator new[](size_t nSize) {
    size_t nRequest = (nSize != 0) ? nSize : 1;
    void *pBlock = HeapAlloc(nRequest);

    if (bMemCountEnabled != 0) {
        ChargeTagTotal(kUntaggedTag, nRequest);
    }
    if (bEnabled != 0) {
        fprintf(fpLog, "new(UNK[],%d,%p)\n", nRequest, pBlock);
    }
    if (pBlock == nullptr) {
        Fatal("NEW[] ALLOCATION FAILURE, size: %d\n", nRequest);
    }
    return pBlock;
}

// NTSC-U/C: 0x004a8520, PAL: 0x004e6630
void *MemAllocTagged(size_t nSize, const char *pszTag, int nLine) {
    void *pBlock = HeapAlloc(nSize);

    if (bMemCountEnabled != 0) {
        ChargeTagTotal(pszTag, nSize);
    }
    if (bEnabled != 0) {
        fprintf(fpLog, "malloc(%s_%d,%d,0x%p)\n", TagBasename(pszTag), nLine, nSize, pBlock);
        strcpy(stlName, kStlUnknownTag);
    }
    if (pBlock == nullptr) {
        Fatal("MEMORY ALLOCATION FAILURE, file: %s, line: %d, size: %d\n", pszTag, nLine, nSize);
    }
    return pBlock;
}

// NTSC-U/C: 0x004a81e0, PAL: 0x004e62f0
void *operator new(size_t nSize) {
    size_t nRequest = (nSize != 0) ? nSize : 1;
    void *pBlock = HeapAlloc(nRequest);

    if (bMemCountEnabled != 0) {
        ChargeTagTotal(kScalarTag, nRequest);
    }
    if (bEnabled != 0) {
        fprintf(fpLog, "new(UNK,%d,%p)\n", nRequest, pBlock);
    }
    if (pBlock == nullptr) {
        Fatal("NEW ALLOCATION FAILURE, size: %d\n", nRequest);
    }
    return pBlock;
}

// NTSC-U/C: 0x004a90a0, PAL: 0x004e71b0
void *AllocateTaggedMemory(size_t nSize, const char *pszClass) {
    size_t nRequest = (nSize != 0) ? nSize : 1;
    void *pBlock = HeapAlloc(nRequest);

    if (bMemCountEnabled != 0) {
        ChargeTagTotal(pszClass, nRequest);
    }
    if (bEnabled != 0) {
        fprintf(fpLog, "new(%s,%d,%p)\n", pszClass, nRequest, pBlock);
    }
    if (pBlock == nullptr) {
        Fatal("NEW ALLOCATION FAILURE, class: %s, size: %d\n", pszClass, nRequest);
    }
    return pBlock;
}

// NTSC-U/C: 0x004a92c8, PAL: 0x004e73d8
// The image, built as C++98, has no sized overload to pair with this one.
void operator delete[](void *pBlock) noexcept {
    if (bEnabled != 0) {
        fprintf(fpLog, "del(UNK[],%p)\n", pBlock);
    }
    HeapFree(pBlock);
}

// NTSC-U/C: 0x004a9230, PAL: 0x004e7340
// The image, built as C++98, has no sized overload to pair with this one.
void operator delete(void *pBlock) noexcept {
    if (bEnabled != 0) {
        fprintf(fpLog, "del(UNK,%p)\n", pBlock);
    }
    HeapFree(pBlock);
}

// NTSC-U/C: 0x004a91e0, PAL: 0x004e72f0
void FreeTaggedMemory(void *pBlock, const char *pszClass) {
    if (bEnabled != 0) {
        fprintf(fpLog, "del(%s,%p)\n", pszClass, pBlock);
    }
    HeapFree(pBlock);
}

// NTSC-U/C: 0x004a94e8, PAL: 0x004e75f8
void MemFreeTagged(void *pBlock, const char *pszTag, int nLine) {
    int nZone = FindZoneForPointer(pBlock);
    if (nZone != kNoZone) {
        Fatal("ZONE FREE ERROR: ptr %p in zone %d, can't free()\n", pBlock, nZone);
    }

    if (bEnabled != 0) {
        fprintf(fpLog, "free(%s_%d,0x%p)\n", TagBasename(pszTag), nLine, pBlock);
    }
    HeapFree(pBlock);
}

// NTSC-U/C: 0x004a93b8, PAL: 0x004e74c8
void *MemReallocTagged(void *pBlock, size_t nSize, const char *pszTag, int nLine) {
    int nZone = FindZoneForPointer(pBlock);
    if (nZone != kNoZone) {
        Fatal("ZONE REALLOC ERROR: ptr %p in zone %d, can't realloc()\n", pBlock, nZone);
    }

    void *pNew = HeapRealloc(pBlock, nSize);
    if (bEnabled != 0) {
        // The log prints the released block's address and never dereferences it.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
        fprintf(fpLog,
                "realloc(%s_%d,%d,0x%p,0x%p)\n",
                TagBasename(pszTag),
                nLine,
                nSize,
                pBlock,
                pNew);
#pragma GCC diagnostic pop
        strcpy(stlName, kStlUnknownTag);
    }
    if (pNew == nullptr) {
        Fatal("MEMORY RE-ALLOCATION FAILURE, file: %s, line: %d, size: %d\n", pszTag, nLine, nSize);
    }
    return pNew;
}

// NTSC-U/C: 0x004a9090, PAL: 0x004e71a0
char *MemGetCurrentTag() {
    return stlName;
}

// NTSC-U/C: 0x004a9048, PAL: 0x004e7158
void MemSetStlTag(const char *pszKind, int nElemSize) {
    sprintf(stlName, "%s.%d", pszKind, nElemSize);
}

// NTSC-U/C: 0x004a8e18, PAL: 0x004e6f28
void MemLogWrite(const char *pszText) {
    if (bEnabled != 0) {
        fprintf(fpLog, "MARKER: %s\n", pszText);
    }
}

// NTSC-U/C: 0x004a86c0, PAL: 0x004e67d0
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

// NTSC-U/C: 0x004a95c8, PAL: 0x004e76d8
void MemLogSourceInit() {
    g_pMemLogBlocks =
        static_cast<MemLogBlock *>(HeapAlloc(kMemLogBlockCount * sizeof(MemLogBlock)));
    if (g_pMemLogBlocks != nullptr) {
        memset(g_pMemLogBlocks, 0, kMemLogBlockCount * sizeof(MemLogBlock));
        memset(g_aMemLogSources, 0, sizeof(g_aMemLogSources));
    }
}

// NTSC-U/C: 0x004a87d0, PAL: 0x004e68e0
void MemLogSourceTrackRealloc(const char *pszSource, void *pNew, void *pOld, int nSize) {
    if (g_pMemLogBlocks == nullptr) {
        return;
    }
    MemLogBlock *pEntry = FindTrackedBlock(pOld);
    if (pEntry == nullptr) {
        LogPrintf("%s(): can't find realloc for src: %s\n", __func__, pszSource);
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

// NTSC-U/C: 0x004a8a68, PAL: 0x004e6b78
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

// NTSC-U/C: 0x004a8e50, PAL: 0x004e6f60
void MemLogPrint(const char *pszText) {
    if (bEnabled != 0) {
        fprintf(fpLog, "%s", pszText);
    }
}

// NTSC-U/C: 0x004a8e88, PAL: 0x004e6f98
void MemBeginAccounting() {
    memCountAlloced = 0;
    memset(memCountSource, 0, sizeof(memCountSource));
    strcpy(memCountSource[0].mName, "Other_Sources");
    bMemCountEnabled = 1;
}

// NTSC-U/C: 0x004a8ef8, PAL: 0x004e7008
int MemEndAccounting(char *pszReport, int nReportSize) {
    sprintf(pszReport, "Memory Allocated: %d\n", memCountAlloced);
    for (int i = 0; i < kMemTagCount; ++i) {
        const MemTagTotal &record = memCountSource[i];
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
    bMemCountEnabled = 0;
    return memCountAlloced;
}

// NTSC-U/C: 0x004a7c40, PAL: 0x004e5d50
void MemOpenLog(const char *pszPath) {
    if (pszPath != nullptr) {
        strcpy(g_szMemLogPath, pszPath);
        strcpy(g_szMemLogBaseName, pszPath);
        g_nMemLogReopenCount = 0;
        fpLog = fopen(g_szMemLogPath, "w");
        if (fpLog != nullptr) {
            bEnabled = 1;
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

// NTSC-U/C: 0x004a7d30, PAL: 0x004e5e40
void MemCloseLogAndReport() {
    if (fpLog != nullptr) {
        fclose(fpLog);
        fpLog = nullptr;
        bEnabled = 0;
    }
    const struct mallinfo info = ReadMallinfo();
    LogPrintf("system heap info (mallinfo):\n");
    LogMallinfo(info);
    LogStackUse();
    DumpHeapMemoryLog(0);
}

// NTSC-U/C: 0x004a7ef8, PAL: 0x004e6008
void MemLogCloseAndContinue() {
    LogPrintf("MemLogCloseAndContinue:, fpLog: %p\n", fpLog);
    if (fpLog != nullptr) {
        fclose(fpLog);
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
        fpLog = fopen(g_szMemLogPath, "w");
        LogPrintf("reopened %s at %p\n", g_szMemLogPath, fpLog);
        char szLine[kMemLogLineSize];
        while (fgets(szLine, kMemLogLineSize, pOld) != nullptr) {
            fputs(szLine, fpLog);
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

// NTSC-U/C: 0x00246c18, PAL: 0x0025bd58
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

// Reports whether a report file may be written. The image writes through and closes the file even
// when fopen() failed. A write through the failed file faults on a console when the game runs from
// a disc.
inline bool CanWriteReport(const FILE *pFile) {
#ifdef ENABLE_PATCHES
    return pFile != nullptr;
#else
    (void)pFile;
    return true;
#endif
}

// NTSC-U/C: 0x0054b348, PAL: 0x0058b878
void DumpHeapMemoryLog(int nIndex) {
    char szPath[kHeapLogPathSize];
    sprintf(szPath, "memdump_%d.txt", nIndex);
    FILE *pFile = fopen(szPath, "w");
    MemLogSourceReport("MEMLOG STATS", pFile);
    if (CanWriteReport(pFile)) {
        fclose(pFile);
    }

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
        if (CanWriteReport(pFile)) {
            fprintf(pFile,
                    "Largest possible allocation: %f megabytes\n",
                    static_cast<float>(nSteps) / kLargestProbeStepsPerMegabyte);
        }
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
        if (CanWriteReport(pFile)) {
            fprintf(pFile,
                    "Able to allocate %d blocks of size %d (%d bytes total)\n",
                    nCount,
                    nBlockSize,
                    nCount * (nBlockSize + kProbeBlockOverhead));
        }
        for (int i = 0; i < nCount; ++i) {
            HeapFree(g_apProbeBlocks[i]);
        }
    }

    if (CanWriteReport(pFile)) {
        fclose(pFile);
    }
}

// NTSC-U/C: 0x004bfd48, PAL: 0x004fdde8
extern "C" void *HeapAlloc(size_t nSize) {
    return malloc(nSize);
}

// NTSC-U/C: 0x004bfd70, PAL: 0x004fde10
extern "C" void HeapFree(void *pBlock) {
    free(pBlock);
}

// NTSC-U/C: 0x00589278, PAL: 0x005cc4f0
extern "C" void *HeapRealloc(void *pBlock, size_t nSize) {
    return realloc(pBlock, nSize);
}
