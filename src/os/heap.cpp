#include "os/heap.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "os/cycles.h"
#include "os/log.h"
#include "os/mem.h"
#include "os/zone.h"

namespace {

// Cycle counts are divided by this before they accumulate, which converts them to microseconds on
// the shipped target.
constexpr int kCyclesPerMicrosecond = 0x127;

// Set while Realloc() drives Alloc() and Free() internally, so that the nested calls do not
// accumulate their own timings on top of the enclosing one.
// NTSC-U/C: 0x00724578, PAL: 0x00768168
int g_bHeapTimingSuspended = 0;

// NTSC-U/C: 0x0072457c, PAL: 0x0076816c
int tMalloc = 0;

// NTSC-U/C: 0x00724580, PAL: 0x00768170
int tRealloc = 0;

// NTSC-U/C: 0x00724584, PAL: 0x00768174
int tFree = 0;

HeapNode *NodePrev(const HeapNode *pNode) {
    return reinterpret_cast<HeapNode *>(pNode->mPrevAndFree & ~static_cast<uintptr_t>(1));
}

bool NodeIsFree(const HeapNode *pNode) {
    return (pNode->mPrevAndFree & 1) != 0;
}

// Writing a pointer clears the free bit, which is how the image releases the tag.
void SetNodePrev(HeapNode *pNode, HeapNode *pPrev) {
    pNode->mPrevAndFree = reinterpret_cast<uintptr_t>(pPrev);
}

void MarkNodeFree(HeapNode *pNode) {
    pNode->mPrevAndFree |= 1;
}

void MarkNodeUsed(HeapNode *pNode) {
    pNode->mPrevAndFree &= ~static_cast<uintptr_t>(1);
}

// The extent spans the prologue and the payload, so it exceeds the payload by
// kHeapNodePrologueSize.
unsigned NodeExtent(const HeapNode *pNode) {
    return reinterpret_cast<const char *>(pNode->mNext) - reinterpret_cast<const char *>(pNode);
}

void *NodePayload(HeapNode *pNode) {
    return reinterpret_cast<char *>(pNode) + kHeapNodePrologueSize;
}

HeapNode *PayloadNode(void *pBlock) {
    return reinterpret_cast<HeapNode *>(static_cast<char *>(pBlock) - kHeapNodePrologueSize);
}

HeapNode *NodeAt(HeapNode *pNode, unsigned nOffset) {
    return reinterpret_cast<HeapNode *>(reinterpret_cast<char *>(pNode) + nOffset);
}

unsigned RoundPayload(unsigned nSize) {
    unsigned nWant = (nSize > kHeapMinPayloadSize - 1) ? nSize : kHeapMinPayloadSize;
    return (nWant + kHeapPayloadAlignment - 1) & ~(kHeapPayloadAlignment - 1);
}

// Realloc() adds its time through this form, which does not test g_bHeapTimingSuspended.
void AddMicroseconds(int *pnTotal, unsigned nStart) {
    *pnTotal += static_cast<int>(ReadCycleCount() - nStart) / kCyclesPerMicrosecond;
}

void AccumulateMicroseconds(int *pnTotal, unsigned nStart) {
    if (g_bHeapTimingSuspended == 0) {
        *pnTotal += static_cast<int>(ReadCycleCount() - nStart) / kCyclesPerMicrosecond;
    }
}

} // namespace

void Heap::UnlinkFreeNode(HeapNode *pNode) {
    if (pNode->mFreePrev != nullptr) {
        pNode->mFreePrev->mFreeNext = pNode->mFreeNext;
    } else {
        mFreeList = pNode->mFreeNext;
    }
    if (pNode->mFreeNext != nullptr) {
        pNode->mFreeNext->mFreePrev = pNode->mFreePrev;
    }
}

void Heap::PushFreeNode(HeapNode *pNode) {
    pNode->mFreePrev = nullptr;
    pNode->mFreeNext = mFreeList;
    if (mFreeList != nullptr) {
        mFreeList->mFreePrev = pNode;
    }
    mFreeList = pNode;
}

void Heap::ReplaceFreeNode(HeapNode *pFrom, HeapNode *pTo) {
    *pTo = *pFrom;
    if (pFrom->mNext != nullptr) {
        SetNodePrev(pFrom->mNext, pTo);
    }
    if (pTo->mFreeNext != nullptr) {
        pTo->mFreeNext->mFreePrev = pTo;
    }
    if (pTo->mFreePrev != nullptr) {
        pTo->mFreePrev->mFreeNext = pTo;
    } else {
        mFreeList = pTo;
    }
}

// NTSC-U/C: 0x00552040, PAL: 0x00592680
Heap *Heap::Create(void *pBlock, unsigned nSize, unsigned nFlags) {
    if (pBlock == nullptr) {
        // Yes, the binary discards this call's result and then builds the heap at address zero.
        // The one caller always supplies a block, so the path never runs.
        MemAllocTagged(nSize, __FILE__, __LINE__);
        nFlags |= kHeapFlagOwnsBlock;
    }

    Heap *pHeap = static_cast<Heap *>(pBlock);
    memset(pHeap, 0, kHeapHeaderSize);

    HeapNode *pFirst = reinterpret_cast<HeapNode *>(static_cast<char *>(pBlock) + kHeapHeaderSize);
    unsigned nLength = nSize - kHeapHeaderSize;
    HeapNode *pTail =
        reinterpret_cast<HeapNode *>(static_cast<char *>(pBlock) + nSize - kHeapNodePrologueSize);

    pFirst->mPrevAndFree = 0;
    pHeap->mStart = pFirst;
    pHeap->mLength = nLength;
    pHeap->mFreeList = pFirst;
    pFirst->mNext = pTail;
    pFirst->mFreePrev = nullptr;
    pFirst->mFreeNext = nullptr;
    MarkNodeFree(pFirst);
    SetNodePrev(pTail, pFirst);
    pTail->mNext = nullptr;

    pHeap->mFlags = nFlags;
    if ((nFlags & (kHeapFlagFirstFit | kHeapFlagBestFit)) == 0) {
        pHeap->mFlags = nFlags | kHeapFlagFirstFit;
    }
    pHeap->mFreeNodes = 1;
    return pHeap;
}

// NTSC-U/C: 0x00552108, PAL: 0x00592748
void Heap::Destroy() {
    if ((mFlags & kHeapFlagOwnsBlock) != 0) {
        MemFreeTagged(this, __FILE__, __LINE__);
    }
}

// NTSC-U/C: 0x00551780, PAL: 0x00591dc0
void *
Heap::Alloc(unsigned nSize, [[maybe_unused]] const char *pszFile, [[maybe_unused]] int nLine) {
    unsigned nStart = ReadCycleCount();
    ++mCallsAlloc;

    unsigned nWant = RoundPayload(nSize);
    unsigned nNeed = nWant + kHeapNodePrologueSize;

    HeapNode *pChosen = nullptr;
    for (HeapNode *pNode = mFreeList; pNode != nullptr; pNode = pNode->mFreeNext) {
        unsigned nExtent = NodeExtent(pNode);
        if (nExtent < nNeed) {
            continue;
        }
        if ((mFlags & kHeapFlagFirstFit) != 0) {
            pChosen = pNode;
            break;
        }
        // The tail node has no successor and is skipped. First-fit tests the flag before this, so
        // only best-fit rules the tail out.
        if (pNode->mNext == nullptr) {
            continue;
        }
        if (pChosen != nullptr && nExtent >= NodeExtent(pChosen)) {
            continue;
        }
        pChosen = pNode;
        if (nExtent == nNeed) {
            break;
        }
    }

    if (pChosen == nullptr) {
        if ((mFlags & kHeapFlagFatalWhenFull) != 0) {
            Fatal("Python heap is out of memory!");
        }
        AccumulateMicroseconds(&tMalloc, nStart);
        return nullptr;
    }

    if (NodeExtent(pChosen) < nWant + kHeapSplitThreshold) {
        UnlinkFreeNode(pChosen);
        MarkNodeUsed(pChosen);
        --mFreeNodes;
    } else {
        HeapNode *pRemainder = NodeAt(pChosen, nNeed);
        SetNodePrev(pRemainder, pChosen);
        pRemainder->mNext = pChosen->mNext;
        if (pChosen->mNext != nullptr) {
            SetNodePrev(pChosen->mNext, pRemainder);
        }
        pRemainder->mFreePrev = pChosen->mFreePrev;
        pRemainder->mFreeNext = pChosen->mFreeNext;
        if (pRemainder->mFreePrev != nullptr) {
            pRemainder->mFreePrev->mFreeNext = pRemainder;
        } else {
            mFreeList = pRemainder;
        }
        if (pRemainder->mFreeNext != nullptr) {
            pRemainder->mFreeNext->mFreePrev = pRemainder;
        }
        MarkNodeFree(pRemainder);
        pChosen->mNext = pRemainder;
        MarkNodeUsed(pChosen);
    }

    ++mUsedNodes;
    mBytes += NodeExtent(pChosen);
    AccumulateMicroseconds(&tMalloc, nStart);
    return NodePayload(pChosen);
}

// NTSC-U/C: 0x00551d28, PAL: 0x00592368
void Heap::Free(void *pBlock, [[maybe_unused]] const char *pszFile, [[maybe_unused]] int nLine) {
    unsigned nStart = ReadCycleCount();
    if (pBlock == nullptr) {
        AccumulateMicroseconds(&tFree, nStart);
        return;
    }

    HeapNode *pNode = PayloadNode(pBlock);
    ++mCallsFree;
    ++mFreeNodes;
    --mUsedNodes;
    mBytes -= NodeExtent(pNode);

    bool bLinked = false;

    HeapNode *pNext = pNode->mNext;
    if (NodeIsFree(pNext)) {
        pNode->mFreeNext = pNext->mFreeNext;
        pNode->mFreePrev = pNext->mFreePrev;
        if (pNode->mFreeNext != nullptr) {
            pNode->mFreeNext->mFreePrev = pNode;
        }
        if (pNode->mFreePrev != nullptr) {
            pNode->mFreePrev->mFreeNext = pNode;
        } else {
            mFreeList = pNode;
        }
        if (pNext->mNext != nullptr) {
            SetNodePrev(pNext->mNext, pNode);
        }
        pNode->mNext = pNext->mNext;
        --mFreeNodes;
        MarkNodeFree(pNode);
        bLinked = true;
    }

    HeapNode *pPrev = NodePrev(pNode);
    if (pPrev != nullptr && NodeIsFree(pPrev)) {
        if (bLinked) {
            UnlinkFreeNode(pNode);
        }
        if (pNode->mNext != nullptr) {
            SetNodePrev(pNode->mNext, pPrev);
        }
        pPrev->mNext = pNode->mNext;
        --mFreeNodes;
        bLinked = true;
    }

    if (!bLinked) {
        MarkNodeFree(pNode);
        PushFreeNode(pNode);
    }

    AccumulateMicroseconds(&tFree, nStart);
}

// NTSC-U/C: 0x005519d0, PAL: 0x00592010
void *Heap::Realloc(void *pBlock,
                    unsigned nSize,
                    [[maybe_unused]] const char *pszFile,
                    [[maybe_unused]] int nLine) {
    unsigned nStart = ReadCycleCount();
    ++mCallsRealloc;

    HeapNode *pNode = PayloadNode(pBlock);
    unsigned nWant = RoundPayload(nSize);
    unsigned nNeed = nWant + kHeapNodePrologueSize;
    HeapNode *pNext = pNode->mNext;
    unsigned nExtent = NodeExtent(pNode);

    if (nExtent >= nNeed) {
        // The block already fits. A remainder too small for a node stays attached.
        if (nExtent - kHeapNodePrologueSize - nWant >= kHeapFreeNodeSize) {
            HeapNode *pRemainder = NodeAt(pNode, nNeed);
            mBytes +=
                (reinterpret_cast<char *>(pRemainder) - static_cast<char *>(pBlock)) - nExtent;
            if (NodeIsFree(pNext)) {
                ReplaceFreeNode(pNext, pRemainder);
                pNode->mNext = pRemainder;
            } else {
                pRemainder->mNext = pNext;
                SetNodePrev(pRemainder, pNode);
                pNode->mNext = pRemainder;
                SetNodePrev(pNext, pRemainder);
                pRemainder->mFreePrev = nullptr;
                pRemainder->mFreeNext = mFreeList;
                if (mFreeList != nullptr) {
                    mFreeList->mFreePrev = pRemainder;
                }
                mFreeList = pRemainder;
                MarkNodeFree(pRemainder);
            }
        }
        AddMicroseconds(&tRealloc, nStart);
        return pBlock;
    }

    if (NodeIsFree(pNext)) {
        unsigned nNextExtent = NodeExtent(pNext);
        unsigned nCombined = nExtent + nNextExtent;
        if (nCombined >= nWant + kHeapHeaderSize) {
            // Enough to grow and still leave a node behind. The split point is one prologue
            // further along than the in-place path uses, so the payload gains eight spare bytes.
            HeapNode *pRemainder = NodeAt(pNode, nWant + kHeapFreeNodeSize);
            mBytes +=
                (reinterpret_cast<char *>(pRemainder) - static_cast<char *>(pBlock)) - nExtent;
            ReplaceFreeNode(pNext, pRemainder);
            pNode->mNext = pRemainder;
            AddMicroseconds(&tRealloc, nStart);
            return pBlock;
        }
        if (nCombined >= nWant + kHeapSplitThreshold) {
            // Absorb the whole following node.
            mBytes += nNextExtent;
            UnlinkFreeNode(pNext);
            SetNodePrev(pNext->mNext, pNode);
            pNode->mNext = pNext->mNext;
            --mFreeNodes;
            AddMicroseconds(&tRealloc, nStart);
            return pBlock;
        }
    }

    // No neighbour can satisfy the growth, so the payload has to move.
    g_bHeapTimingSuspended = 1;
    void *pMoved = Alloc(nWant, __FILE__, __LINE__);
    if (pMoved == nullptr) {
        g_bHeapTimingSuspended = 0;
        AddMicroseconds(&tRealloc, nStart);
        return nullptr;
    }
    memcpy(pMoved, pBlock, NodeExtent(pNode) - kHeapNodePrologueSize);
    Free(pBlock, __FILE__, __LINE__);
    g_bHeapTimingSuspended = 0;
    AddMicroseconds(&tRealloc, nStart);
    return pMoved;
}

// NTSC-U/C: 0x00552138, PAL: 0x00592778
int Heap::Shrink(unsigned nSize) {
    if (nSize >= mLength) {
        return 0;
    }

    HeapNode *pTail = mStart;
    while (pTail->mNext != nullptr) {
        pTail = pTail->mNext;
    }

    HeapNode *pLast = NodePrev(pTail);
    if (!NodeIsFree(pLast)) {
        return 0;
    }
    unsigned nSpare = NodeExtent(pLast) - kHeapFreeNodeSize;
    if (mLength - nSpare >= nSize) {
        return 0;
    }

    // The reallocation result is discarded, so shortening relies on the block staying put.
    MemReallocTagged(this, nSize + kHeapHeaderSize, __FILE__, __LINE__);
    mLength = nSize;
    HeapNode *pNewTail = reinterpret_cast<HeapNode *>(reinterpret_cast<char *>(mStart) + nSize -
                                                      kHeapNodePrologueSize);
    SetNodePrev(pNewTail, pLast);
    pNewTail->mNext = nullptr;
    pLast->mNext = pNewTail;
    return 1;
}

// NTSC-U/C: 0x00551ec8, PAL: 0x00592508
void Heap::DumpToFile(const char *pszPath) {
    FILE *pFile = fopen(pszPath, "w");
    if (pFile == nullptr) {
        return;
    }

    fprintf(pFile, "Heap Info: length: %d flags: %d\n", mLength, mFlags);
    fprintf(pFile,
            "HHeap Stats: nUsedNodes:%d nFreeNodes:%d nCalls:(M:%d R:%d F:%d) nBytes:%d\n",
            mUsedNodes,
            mFreeNodes,
            mCallsAlloc,
            mCallsRealloc,
            mCallsFree,
            mBytes);
    LogPrintf("HHeap Stats: nUsedNodes:%d nFreeNodes:%d nCalls:(M:%d R:%d F:%d) nBytes:%d\n",
              mUsedNodes,
              mFreeNodes,
              mCallsAlloc,
              mCallsRealloc,
              mCallsFree,
              mBytes);
    fprintf(pFile, "Timing:(M:%d R:%d F:%d)\n", tMalloc, tRealloc, tFree);
    LogPrintf("Timing:(M:%d R:%d F:%d)\n", tMalloc, tRealloc, tFree);
    fprintf(pFile, "ptr  len  isfree\n");

    for (HeapNode *pNode = mStart; pNode != nullptr && pNode->mNext != nullptr;
         pNode = pNode->mNext) {
        fprintf(pFile, "0x%p %d %d\n", pNode, NodeExtent(pNode), NodeIsFree(pNode) ? 1 : 0);
    }

    fclose(pFile);
}

// NTSC-U/C: 0x00552218, PAL: 0x00592858
void Heap::DumpStats() {
    char szLine[0xa0];
    sprintf(szLine,
            "HHeap Stats: nUsedNodes:%d nFreeNodes:%d nCalls:(M:%d R:%d F:%d) nBytes:%d",
            mUsedNodes,
            mFreeNodes,
            mCallsAlloc,
            mCallsRealloc,
            mCallsFree,
            mBytes);
    MemLogWrite(szLine);
    LogPrintf("%s\n", szLine);
}

// NTSC-U/C: 0x00723998, PAL: 0x00767588
Heap *gpPythonHeap = nullptr;

namespace {

// The zone the interpreter heap takes whole, and the size ZoneGetAvail() reports when the zone is
// not found.
constexpr char kPythonZoneName[] = "python";
constexpr int kPythonHeapFallbackSize = 0x200000;

} // namespace

// NTSC-U/C: 0x0054d55c, PAL: 0x0058dafc
// The heap setup Py_Initialize() runs first, before the interpreter allocates anything.
extern "C" void PyHeap_Init(void) {
    const int nPreviousZone = ZoneGetCurrent();
    ZoneSetCurrent(FindZoneByName(kPythonZoneName));
    const int nSize = ZoneGetAvail(kPythonHeapFallbackSize);
    void *pBlock = ZoneAlloc(static_cast<unsigned>(nSize));
    gpPythonHeap = Heap::Create(
        pBlock, static_cast<unsigned>(nSize), kHeapFlagFirstFit | kHeapFlagFatalWhenFull);
    ZoneSetCurrent(nPreviousZone);
}

extern "C" void *PyHeap_Alloc(unsigned nSize, const char *pszFile, int nLine) {
    return gpPythonHeap->Alloc(nSize, pszFile, nLine);
}

extern "C" void *PyHeap_Realloc(void *pBlock, unsigned nSize, const char *pszFile, int nLine) {
    return gpPythonHeap->Realloc(pBlock, nSize, pszFile, nLine);
}

extern "C" void PyHeap_Free(void *pBlock, const char *pszFile, int nLine) {
    gpPythonHeap->Free(pBlock, pszFile, nLine);
}
