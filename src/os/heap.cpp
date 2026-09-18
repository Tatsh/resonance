#include "os/heap.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "os/cycles.h"
#include "os/log.h"
#include "os/mem.h"

namespace {

// Cycle counts are divided by this before they accumulate, which converts them to microseconds on
// the shipped target.
constexpr int kCyclesPerMicrosecond = 0x127;

// 0x00724578. Set while Realloc() drives Alloc() and Free() internally, so that the nested calls
// do not accumulate their own timings on top of the enclosing one.
int g_bHeapTimingSuspended;

// 0x0072457c
int g_nHeapAllocMicroseconds;

// 0x00724580
int g_nHeapReallocMicroseconds;

// 0x00724584
int g_nHeapFreeMicroseconds;

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

void AccumulateMicroseconds(int *pnTotal, unsigned nStart) {
    if (g_bHeapTimingSuspended == 0) {
        *pnTotal += (ReadCycleCount() - nStart) / kCyclesPerMicrosecond;
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

void Heap::Destroy() {
    if ((mFlags & kHeapFlagOwnsBlock) != 0) {
        MemFreeTagged(this, __FILE__, __LINE__);
    }
}

void *Heap::Alloc(unsigned nSize, const char *pszFile, int nLine) {
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
            Fatal("Python heap is out of memory!\n");
        }
        AccumulateMicroseconds(&g_nHeapAllocMicroseconds, nStart);
        return nullptr;
    }

    if (NodeExtent(pChosen) < nWant + kHeapSplitThreshold) {
        UnlinkFreeNode(pChosen);
        MarkNodeUsed(pChosen);
        --mFreeNodes;
    } else {
        HeapNode *pRemainder = NodeAt(pChosen, nNeed);
        SetNodePrev(pRemainder, pChosen);
        if (pChosen->mNext != nullptr) {
            pRemainder->mNext = pChosen->mNext;
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
    AccumulateMicroseconds(&g_nHeapAllocMicroseconds, nStart);
    return NodePayload(pChosen);
}

void Heap::Free(void *pBlock, const char *pszFile, int nLine) {
    unsigned nStart = ReadCycleCount();
    if (pBlock == nullptr) {
        AccumulateMicroseconds(&g_nHeapFreeMicroseconds, nStart);
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

    AccumulateMicroseconds(&g_nHeapFreeMicroseconds, nStart);
}

void *Heap::Realloc(void *pBlock, unsigned nSize, const char *pszFile, int nLine) {
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
        AccumulateMicroseconds(&g_nHeapReallocMicroseconds, nStart);
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
            AccumulateMicroseconds(&g_nHeapReallocMicroseconds, nStart);
            return pBlock;
        }
        if (nCombined >= nWant + kHeapSplitThreshold) {
            // Absorb the whole following node.
            mBytes += nNextExtent;
            UnlinkFreeNode(pNext);
            SetNodePrev(pNext->mNext, pNode);
            pNode->mNext = pNext->mNext;
            --mFreeNodes;
            AccumulateMicroseconds(&g_nHeapReallocMicroseconds, nStart);
            return pBlock;
        }
    }

    // No neighbour can satisfy the growth, so the payload has to move.
    g_bHeapTimingSuspended = 1;
    void *pMoved = Alloc(nWant, __FILE__, __LINE__);
    if (pMoved == nullptr) {
        g_bHeapTimingSuspended = 0;
        AccumulateMicroseconds(&g_nHeapReallocMicroseconds, nStart);
        return nullptr;
    }
    memcpy(pMoved, pBlock, NodeExtent(pNode) - kHeapNodePrologueSize);
    Free(pBlock, __FILE__, __LINE__);
    g_bHeapTimingSuspended = 0;
    AccumulateMicroseconds(&g_nHeapReallocMicroseconds, nStart);
    return pMoved;
}

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
    fprintf(pFile,
            "Timing:(M:%d R:%d F:%d)\n",
            g_nHeapAllocMicroseconds,
            g_nHeapReallocMicroseconds,
            g_nHeapFreeMicroseconds);
    LogPrintf("Timing:(M:%d R:%d F:%d)\n",
              g_nHeapAllocMicroseconds,
              g_nHeapReallocMicroseconds,
              g_nHeapFreeMicroseconds);
    fprintf(pFile, "ptr  len  isfree\n");

    for (HeapNode *pNode = mStart; pNode != nullptr && pNode->mNext != nullptr;
         pNode = pNode->mNext) {
        fprintf(pFile, "0x%p %d %d\n", pNode, NodeExtent(pNode), NodeIsFree(pNode) ? 1 : 0);
    }

    fclose(pFile);
}

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
