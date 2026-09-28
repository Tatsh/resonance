#include <stddef.h>
#include <stdint.h>

#include <libdev.h>

// One debug console, 0x58 bytes long. The buffer holds one 16 bit cell per character, while the
// trailing words hold the console state that opening initialises.
typedef struct {
    int nColumns;
    int nRows;
    unsigned short *pBuffer;
    unsigned char nAttribute;
    unsigned char mPadding0d[3];
    int mUnknown10;
    int mUnknown14;
    unsigned int mContext[16];
} DevConsole;

enum {
    kConsoleCount = 4,
    kColumnLimit = 0x51,
    kRowLimit = 0x41,
    kDefaultAttribute = 7,
    kClearCell = 0x720,
    kHeapWords = 0x2800
};

// The console slots. A null buffer marks a free slot.
// 0x008e6980
static DevConsole g_devConsoles[kConsoleCount];

// The console heap. An all-ones size in the first header marks the heap as not yet initialised.
// 0x007a8530
static unsigned int g_anDevHeap[kHeapWords] __attribute__((aligned(16))) = {0xffffffffu};

// Fill the console state words with their defaults, and record the GS primitive position.
static void initConsoleContext(unsigned int *pContext, unsigned int nGsX, unsigned int nGsY);

// Take nSize bytes from the static heap, returning null when no block fits. The first call
// initialises the heap and its end marker.
static void *heapAllocate(unsigned int nSize);

// 0x0062bfd0
void sceDevVif0Reset(void) {
    *(volatile unsigned int *)0x10003810u = 1u;
    *(volatile unsigned int *)0x10003820u = 6u;
}

// 0x0061dc90
void sceDevVu0Reset(void) {
    unsigned int nStatus;

    __asm__ volatile ("cfc2 %0, $vi12" : "=r" (nStatus));
    nStatus |= 2u;
    __asm__ volatile ("ctc2 %0, $vi12" : : "r" (nStatus));
}

// 0x00622710
void sceDevConsInit(void) {
    DevConsole *pConsoles = g_devConsoles;
    int nIndex;

    for (nIndex = 0; nIndex < kConsoleCount; nIndex++) {
        pConsoles[nIndex].pBuffer = NULL;
    }
}

// 0x00622748
int sceDevConsOpen(
    unsigned int nGsX, unsigned int nGsY, unsigned int nColumns, unsigned int nRows) {
    DevConsole *pConsoles = g_devConsoles;
    DevConsole *pConsole = NULL;
    int nIndex;

    if (nColumns < kColumnLimit && nRows < kRowLimit) {
        for (nIndex = 0; nIndex < kConsoleCount; nIndex++) {
            if (pConsoles[nIndex].pBuffer == NULL) {
                pConsole = &pConsoles[nIndex];
                break;
            }
        }
        if (pConsole != NULL) {
            pConsole->pBuffer = heapAllocate(nColumns * nRows * 2u);
            if (pConsole->pBuffer != NULL) {
                pConsole->nColumns = (int)nColumns;
                pConsole->nRows = (int)nRows;
                pConsole->nAttribute = kDefaultAttribute;
                initConsoleContext(pConsole->mContext, nGsX, nGsY);
                sceDevConsClear((int)(uintptr_t)pConsole);
                return (int)(uintptr_t)pConsole;
            }
        }
    }
    return 0;
}

// 0x00622c98
void sceDevConsClear(int nConsole) {
    DevConsole *pConsole = (DevConsole *)(uintptr_t)nConsole;
    int nRemaining = pConsole->nColumns * pConsole->nRows - 1;
    unsigned short *pCell = pConsole->pBuffer;

    while (nRemaining != -1) {
        *pCell++ = kClearCell;
        --nRemaining;
    }
    pConsole->mUnknown14 = 0;
    pConsole->mUnknown10 = 0;
}

// 0x00622610
static void initConsoleContext(unsigned int *pContext, unsigned int nGsX, unsigned int nGsY) {
    pContext[0] = nGsX;
    pContext[1] = nGsY;
    pContext[2] = 0x80u;
    pContext[3] = 0x80u;
    pContext[4] = 0x10u;
    pContext[5] = 0u;
    pContext[6] = 0u;
    pContext[7] = 0x3cu;
    pContext[8] = 0x80000000u;
    pContext[9] = 0x80ff0000u;
    pContext[10] = 0x800000ffu;
    pContext[11] = 0x80ff00ffu;
    pContext[12] = 0x8000ff00u;
    pContext[13] = 0x80ffff00u;
    pContext[14] = 0x8000ffffu;
    pContext[15] = 0x80ffffffu;
}

// 0x00623b10
static void *heapAllocate(unsigned int nSize) {
    unsigned int *pHeap = g_anDevHeap;
    unsigned int nWords = (nSize + 3u) >> 2;
    unsigned int nIndex = 0u;
    unsigned int nOffset = 0u;
    unsigned int nHeader;
    unsigned int nBlockSize;
    void *pResult = NULL;

    if ((*pHeap & 0x0fffffffu) == 0x0fffffffu) {
        unsigned long long *pHeapWide = (unsigned long long *)pHeap;
        unsigned char *pHeapEnd = (unsigned char *)pHeap + 0x8000u;
        unsigned long long *pEndMark = (unsigned long long *)(pHeapEnd + 0x1ff8u);
        unsigned long long nHeap = *pHeapWide & 0xfffffffff0000000ull;
        unsigned long long nMark = *pEndMark & 0xf0000000ffffffffull;

        nHeap |= 0x27feull;
        nHeap &= 0xffffffff0fffffffull;
        *pHeapWide = nHeap;
        nMark &= 0x0fffffffffffffffull;
        nMark |= 0x3000000000000000ull;
        *pEndMark = nMark;
    }
    if ((*pHeap >> 28) != 3u) {
        for (;;) {
            unsigned int *pHeader = (unsigned int *)((unsigned char *)pHeap + nOffset);

            nHeader = *pHeader;
            nBlockSize = nHeader & 0x0fffffffu;
            if ((nHeader >> 28) == 0u && nBlockSize >= nWords) {
                if (nBlockSize != nWords) {
                    unsigned int *pSplit;

                    nBlockSize -= nWords + 1u;
                    pSplit = (unsigned int *)((unsigned char *)pHeap + (nIndex + nWords + 1u) * 4u);
                    *pSplit = nBlockSize;
                    nHeader = (nHeader & 0xf0000000u) | nWords;
                    *pHeader = nHeader;
                }
                pResult = pHeader + 1;
                *pHeader = (nHeader & 0x0fffffffu) | 0x10000000u;
                break;
            }
            nIndex += nBlockSize + 1u;
            nOffset = nIndex << 2;
            if ((*((unsigned int *)((unsigned char *)pHeap + nOffset)) >> 28) == 3u) {
                break;
            }
        }
    }
    return pResult;
}
