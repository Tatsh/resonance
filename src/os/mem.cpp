#include "os/mem.h"

#include <stdio.h>
#include <string.h>

#include "os/log.h"
#include "os/zone.h"

namespace {

// The interned source table MemLogFindSource() matches against.
constexpr int kMemLogSourceCount = 128;
constexpr int kMemLogSourceNameSize = 64;

// A source name of this length or more is rejected, which leaves each record
// spare room.
constexpr int kMemLogSourceNameLimit = 40;

// The tag the untagged array allocation path bills to.
constexpr char kUntaggedTag[] = "UNK[]";

// The tag the untagged single-object allocation path bills to.
constexpr char kScalarTag[] = "UNK";

// The tag the STL allocator hook is rewound to after every tagged allocation.
constexpr char kStlUnknownTag[] = "stl_unk";

// 0x006f57d0
int g_bMemLogging;

// 0x006f57d8. The rewind to kStlUnknownTag compiles to one doubleword store. The array bound
// comes from the distance to the next global rather than from any single access.
char g_szStlAllocTag[kMemStlTagSize];

// 0x006f5858
int g_bMemAccounting;

// 0x006f585c
int g_nMemTotalBytes;

// 0x006f5860
MemTagTotal g_aMemTagTotals[kMemTagCount];

// 0x006f6460
char g_aMemLogSourceNames[kMemLogSourceCount][kMemLogSourceNameSize];

// 0x00894d70
FILE *g_pMemLogFile;

// Reduces a tag to the text after its last path separator. Both separators are
// tried, so a tag recorded on a Windows build host still logs as a basename.
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

} // namespace

void *MemAlloc(size_t nSize) {
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

void *MemAllocScalar(size_t nSize) {
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

void MemFree(void *pBlock) {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "del(UNK[],%p)\n", pBlock);
    }
    HeapFree(pBlock);
}

void MemFreeScalar(void *pBlock) {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "del(UNK,%p)\n", pBlock);
    }
    HeapFree(pBlock);
}

void FreeTaggedMemory(void *pBlock, const char *pszClass) {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "del(%s,%p)\n", pszClass, pBlock);
    }
    HeapFree(pBlock);
}

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

char *MemGetCurrentTag() {
    return g_szStlAllocTag;
}

void MemSetStlTag(const char *pszKind, int nElemSize) {
    sprintf(g_szStlAllocTag, "%s.%d", pszKind, nElemSize);
}

void MemLogWrite(const char *pszText) {
    if (g_bMemLogging != 0) {
        fprintf(g_pMemLogFile, "MARKER: %s\n", pszText);
    }
}

int MemLogFindSource(const char *pszName) {
    const char *pName = TagBasename(pszName);

    int nRow = 0;
    if (g_aMemLogSourceNames[0][0] != '\0') {
        for (;;) {
            if (strcmp(g_aMemLogSourceNames[nRow], pName) == 0) {
                return nRow;
            }
            ++nRow;
            if (nRow >= kMemLogSourceCount) {
                break;
            }
            if (g_aMemLogSourceNames[nRow][0] == '\0') {
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
    strcpy(g_aMemLogSourceNames[nRow], pName);
    return nRow;
}

// HeapAlloc(), HeapFree(), and HeapRealloc() have no bodies here. Each is two instructions that
// load the toolchain allocator's state from 0x007819cc and tail-call it, and the allocator itself
// is toolchain code that this tree does not reconstruct.
