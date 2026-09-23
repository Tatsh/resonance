#include "gfx/vramtable.h"

#include <eekernel.h>
#include <string.h>

#include "gfx/gfxdevice.h"
#include "os/log.h"
#include "rndartt/abitmap.h"

// 0x0070d3d0
const int g_anGsPixelStorageModes[kABitmapFormatCount] = {
    kGsPsmT4, kGsPsmT8, kGsPsmCt16, kGsPsmCt24, kGsPsmCt32, kGsPsmT8};

// 0x0070d3e8
const int g_anBitsPerPixelTable[kABitmapFormatCount] = {4, 8, 16, 24, 32, 8};

// 0x005149f0
VramTable::~VramTable() {
}

// 0x00512850
void VramTable::Init() {
    mFlushCount = 0;
    mPaletteBase = g_gfxDevice.GetReservedVramWords() / (kVramBlockBytes / 4);
    mPaletteEnd = mPaletteBase + kVramPalAreaBlocks;

    for (int nEntry = 0; nEntry < kVramTableEntries; ++nEntry) {
        g_vramEntries[nEntry].mpPrev = (nEntry != 0) ? &g_vramEntries[nEntry - 1] : nullptr;
        g_vramEntries[nEntry].mpNext =
            (nEntry != kVramTableEntries - 1) ? &g_vramEntries[nEntry + 1] : nullptr;
    }
    mpListTail[kVramListPool] = &g_vramEntries[kVramTableEntries - 1];
    mpListHead[kVramListPool] = &g_vramEntries[0];
    mpChainTail = nullptr;
    mpChainHead = nullptr;
    mpListTail[kVramListFree] = nullptr;
    mpListHead[kVramListFree] = nullptr;
    mpListTail[kVramListUsed] = nullptr;
    mpListHead[kVramListUsed] = nullptr;

    g_pVramPalFree = &g_vramPalEntries[0];
    g_pVramPalUsed = nullptr;
    for (int nPal = 0; nPal < kVramPalEntries; ++nPal) {
        g_vramPalEntries[nPal].mpNext =
            (nPal != kVramPalEntries - 1) ? &g_vramPalEntries[nPal + 1] : nullptr;
    }

    Clear(0);
}

// 0x00512970
void VramTable::Clear(int bClearPalettes) {
    VramTableEntry *pEntry = mpListHead[kVramListFree];
    while (pEntry != nullptr) {
        VramTableEntry *pNext = pEntry->mpNext;
        ReleaseEntry(pEntry);
        pEntry = pNext;
    }

    for (pEntry = mpListHead[kVramListUsed]; pEntry != nullptr; pEntry = pEntry->mpNext) {
        if ((pEntry->mLockMask & kVramLockGenerationMask) != 0) {
            LogPrintf("Clear() - resetting lock on entry %d\n",
                      static_cast<int>(pEntry - g_vramEntries));
        }
        pEntry->mMemAddr = 0;
        pEntry->mLockMask =
            static_cast<unsigned char>(pEntry->mLockMask & ~kVramLockGenerationMask);
    }

    VramTableEntry *pWhole = AllocEntry();
    pWhole->MakeFree(static_cast<unsigned short>(mPaletteEnd),
                     static_cast<unsigned short>(kVramBlocks - mPaletteEnd));
    pWhole->mpPrev = nullptr;
    pWhole->mpNext = nullptr;
    pWhole->mpLower = nullptr;
    pWhole->mpUpper = nullptr;
    mpListHead[kVramListFree] = pWhole;
    mpChainTail = pWhole;
    mpChainHead = pWhole;
    mpListTail[kVramListFree] = pWhole;

    for (int nGeneration = 0; nGeneration < kVramLockGenerations; ++nGeneration) {
        g_anVramLockCount[nGeneration] = 0;
    }
    mBlocksInUse = 0;
    mLockGeneration = 1;

    if (bClearPalettes != 0) {
        VramPalEntry *pPal = g_pVramPalUsed;
        while (pPal != nullptr) {
            VramPalEntry *pNextPal = pPal->mpNext;
            if (pNextPal != nullptr) {
                pNextPal->mpPrev = pPal->mpPrev;
            }
            if (pPal->mpPrev != nullptr) {
                pPal->mpPrev->mpNext = pPal->mpNext;
            } else {
                g_pVramPalUsed = pNextPal;
            }
            const int nSlot = pPal->GetSlotIndex();
            g_adVramPalSlots[nSlot / kVramPalSlotsPerWord] &= ~(1u << nSlot);
            pPal->mMemAddr = 0;
            pPal = pNextPal;
        }
    }
}

// 0x00514cd8
void VramTable::BeginFrame() {
}

// 0x00513190
void VramTable::EndFrame() {
    for (VramTableEntry *pEntry = mpListHead[kVramListUsed]; pEntry != nullptr;
         pEntry = pEntry->mpNext) {
        pEntry->mLockMask =
            static_cast<unsigned char>(pEntry->mLockMask & ~kVramLockGenerationMask);
    }
    // The walk starts at the first record of the array rather than at the resident chain, so it
    // covers whatever the array's own links reach from there.
    for (VramPalEntry *pPal = &g_vramPalEntries[0]; pPal != nullptr; pPal = pPal->mpNext) {
        pPal->mLockMask = static_cast<unsigned short>(pPal->mLockMask & ~kVramLockGenerationMask);
    }
    for (int nGeneration = 0; nGeneration < kVramLockGenerations; ++nGeneration) {
        g_anVramLockCount[nGeneration] = 0;
        g_anVramPalLockCount[nGeneration] = 0;
    }

    mAccumSwaps += mSwaps;
    mAccumMisses += mMisses;
    mAccumFreeMerges += mFreeMerges;
    mAccumLoads += mLoads;
    mAccumLoadBlocks += mLoadBlocks;
    g_nVramLoadsLastFrame = mLoads;
    g_nVramLoadBlocksLastFrame = mLoadBlocks;
    memset(&mSwaps, 0, kVramFrameCounterBytes);
}

// 0x00513298
void VramTable::AdvanceLockCycle() {
    // The counter is advanced through the file scope instance rather than through this.
    ++g_vramTable.mFlushCount;

    mLockGeneration = static_cast<unsigned char>(mLockGeneration + 1);
    if (mLockGeneration > kVramLockGenerations) {
        mLockGeneration = 1;
    }

    const int nGeneration = mLockGeneration - 1;
    const int nBit = 1 << nGeneration;

    VramTableEntry **ppEntry = g_apVramLocked[nGeneration];
    for (int nLocked = g_anVramLockCount[nGeneration]; nLocked > 0; --nLocked) {
        if (((*ppEntry)->mLockMask & nBit) == 0) {
            LogPrintf("VRAM should have lockmask %d set but instead has %d\n",
                      nBit,
                      (*ppEntry)->mLockMask);
        }
        (*ppEntry)->mLockMask = static_cast<unsigned char>((*ppEntry)->mLockMask & ~nBit);
        ++ppEntry;
    }

    VramPalEntry **ppPal = g_apVramPalLocked[nGeneration];
    for (int nPalLocked = g_anVramPalLockCount[nGeneration]; nPalLocked > 0; --nPalLocked) {
        if (((*ppPal)->mLockMask & nBit) == 0) {
            LogPrintf("VRAM pal entry should have lockmask %d set but instead has %d\n",
                      nBit,
                      (*ppPal)->mLockMask);
        }
        (*ppPal)->mLockMask = static_cast<unsigned short>((*ppPal)->mLockMask & ~nBit);
        ++ppPal;
    }

    g_anVramLockCount[nGeneration] = 0;
    g_anVramPalLockCount[nGeneration] = 0;
}

// 0x00513438
void VramTable::PrintStats(const char *pszPrefix) {
    LogPrintf("%sVRAM in use: %d blocks (%d bytes)\n",
              pszPrefix,
              mBlocksInUse,
              mBlocksInUse * kVramBlockBytes);

    const int nRequests = (mRequests != 0) ? mRequests : 1;
    const int nMissPercent =
        static_cast<int>(static_cast<float>(mMisses) * 100.0f / static_cast<float>(nRequests));
    const int nSwapPercent =
        static_cast<int>(static_cast<float>(mSwaps) * 100.0f / static_cast<float>(nRequests));
    LogPrintf("%sVRAM stats (frame):  loads: %d (%d blocks), misses: %d (%d%%), "
              "swaps: %d (%d%%), freemerge: %d\n",
              pszPrefix,
              mLoads,
              mLoadBlocks,
              mMisses,
              nMissPercent,
              mSwaps,
              nSwapPercent,
              mFreeMerges);
    LogPrintf("%sVRAM stats (accum):  loads: %d (%d blocks), misses: %d, swaps: %d, "
              "freemerge: %d\n",
              pszPrefix,
              mAccumLoads,
              mAccumLoadBlocks,
              mAccumMisses,
              mAccumSwaps,
              mAccumFreeMerges);
}

// 0x00514ce0
void VramTable::GetLastFrameLoads(int *pnLoads, int *pnBlocks) const {
    *pnLoads = g_nVramLoadsLastFrame;
    *pnBlocks = g_nVramLoadBlocksLastFrame;
}

// 0x005149f8
VramTableEntry *VramTable::AllocEntry() {
    VramTableEntry *pEntry = mpListHead[kVramListPool];
    if (pEntry == nullptr) {
        LogPrintf("Out of VRAM Table Entries!!!\n");
        return nullptr;
    }

    VramTableEntry *pNext = pEntry->mpNext;
    mpListHead[kVramListPool] = pNext;
    if (pNext != nullptr) {
        pNext->mpPrev = nullptr;
    }
    if (mpListTail[kVramListPool] == pEntry) {
        mpListTail[kVramListPool] = nullptr;
    }
    return pEntry;
}

// 0x00515360
VramPalEntry *VramTable::AllocPalEntry() {
    VramPalEntry *pPal = g_pVramPalFree;
    if (pPal == nullptr) {
        Fatal("Exceeded %d available VRAM palette entries!\n", kVramPalEntries);
    }
    g_pVramPalFree = pPal->mpNext;
    return pPal;
}

// 0x00514a48
void VramTable::UnlinkEntry(VramTableEntry *pEntry, int nList) {
    if (pEntry->mpPrev == nullptr) {
        mpListHead[nList] = pEntry->mpNext;
    } else {
        pEntry->mpPrev->mpNext = pEntry->mpNext;
    }
    if (pEntry->mpNext == nullptr) {
        mpListTail[nList] = pEntry->mpPrev;
    } else {
        pEntry->mpNext->mpPrev = pEntry->mpPrev;
    }
}

// 0x00514b60
void VramTable::ReleaseEntry(VramTableEntry *pEntry) {
    if (mpListHead[kVramListPool] == nullptr) {
        mpListHead[kVramListPool] = pEntry;
        mpListTail[kVramListPool] = pEntry;
        pEntry->mpNext = nullptr;
        pEntry->mpPrev = nullptr;
    } else {
        pEntry->mpPrev = nullptr;
        pEntry->mpNext = mpListHead[kVramListPool];
        mpListHead[kVramListPool]->mpPrev = pEntry;
        mpListHead[kVramListPool] = pEntry;
    }
    pEntry->Reset();
}

// 0x00514bb8
void VramTable::MergeBlocks(VramTableEntry *pKeep, VramTableEntry *pAbsorb) {
    pKeep->mSize = static_cast<unsigned short>(pKeep->mSize + pAbsorb->mSize);
    UnlinkEntry(pAbsorb, kVramListFree);
    ReleaseEntry(pAbsorb);
    pKeep->mpUpper = pAbsorb->mpUpper;
    if (pAbsorb->mpUpper != nullptr) {
        pAbsorb->mpUpper->mpLower = pKeep;
    } else {
        mpChainTail = pKeep;
    }
    ++mFreeMerges;
}

// 0x00512b50
void VramTable::FreeBlock(VramTableEntry *pEntry) {
    if (mpListHead[kVramListFree] == nullptr) {
        mpListTail[kVramListFree] = pEntry;
        mpListHead[kVramListFree] = pEntry;
        pEntry->mpNext = nullptr;
        pEntry->mpPrev = nullptr;
    } else {
        pEntry->mpPrev = nullptr;
        pEntry->mpNext = mpListHead[kVramListFree];
        mpListHead[kVramListFree]->mpPrev = pEntry;
        mpListHead[kVramListFree] = pEntry;
    }
    pEntry->mKind = kVramBlockKindFree;

    VramTableEntry *pUpper = pEntry->mpUpper;
    if (pUpper != nullptr && pUpper->mKind == kVramBlockKindFree) {
        MergeBlocks(pEntry, pUpper);
    }
    VramTableEntry *pLower = pEntry->mpLower;
    if (pLower != nullptr && pLower->mKind == kVramBlockKindFree) {
        MergeBlocks(pLower, pEntry);
    }
}

// 0x00512cf0
void VramTable::RemoveFromChain(VramTableEntry *pEntry) {
    if (pEntry->mpUpper != nullptr) {
        pEntry->mpUpper->mpLower = pEntry->mpLower;
    } else {
        mpChainTail = pEntry->mpLower;
    }
    if (pEntry->mpLower != nullptr) {
        pEntry->mpLower->mpUpper = pEntry->mpUpper;
    } else {
        mpChainHead = pEntry->mpUpper;
    }

    VramTableEntry *pLower = pEntry->mpLower;
    VramTableEntry *pUpper = pEntry->mpUpper;
    if (pLower != nullptr && pUpper != nullptr && pLower->mKind == kVramBlockKindFree &&
        pUpper->mKind == pLower->mKind) {
        MergeBlocks(pLower, pUpper);
    }
}

// 0x00512e08
void VramTable::AllocBlock(VramTableEntry *pEntry, unsigned short nBlocks) {
    VramTableEntry *pTaken = nullptr;

    while (pTaken == nullptr) {
        int nBestSize = 0;
        int nBestAge = 0;

        for (VramTableEntry *pFree = mpListHead[kVramListFree]; pFree != nullptr;
             pFree = pFree->mpNext) {
            if (pFree->mSize == nBlocks) {
                pTaken = pFree;
                break;
            }
            if (pFree->mSize > nBlocks && (pTaken == nullptr || pFree->mSize < nBestSize)) {
                nBestSize = pFree->mSize;
                pTaken = pFree;
            }
        }
        if (pTaken != nullptr) {
            break;
        }

        VramTableEntry *pUsed = mpListHead[kVramListUsed];
        for (VramTableEntry *pScan = pUsed; pScan != nullptr; pScan = pScan->mpNext) {
            if (pScan->mLockMask != 0 || pScan->mMemAddr == 0) {
                continue;
            }
            const int nAge = mFlushCount - pScan->mLastUsed;
            if (pScan->mSize >= nBlocks && (pTaken == nullptr || nBestAge < nAge)) {
                nBestAge = nAge;
                pTaken = pScan;
            }
        }
        if (pTaken != nullptr) {
            break;
        }

        // Nothing is both large enough and unlocked, so release resident blocks until several
        // times the request has come free and then search again.
        int nFreed = 0;
        VramTableEntry *pScan = pUsed;
        while (pScan != nullptr) {
            if (pScan->mLockMask != 0) {
                pScan = pScan->mpNext;
                continue;
            }
            VramTableEntry *pNextUsed = pScan->mpNext;
            if (pScan->mMemAddr == 0) {
                pScan = pNextUsed;
                continue;
            }

            nFreed += pScan->mSize;
            // A null record here is dereferenced by MakeFree(). Yes, the binary does that.
            VramTableEntry *pHole = AllocEntry();
            pHole->MakeFree(pScan->mMemAddr, pScan->mSize);
            if (pScan->mpLower != nullptr) {
                pScan->mpLower->mpUpper = pHole;
            } else {
                mpChainHead = pHole;
            }
            if (pScan->mpUpper != nullptr) {
                pScan->mpUpper->mpLower = pHole;
            } else {
                mpChainTail = pHole;
            }
            pHole->mpUpper = pScan->mpUpper;
            pHole->mpLower = pScan->mpLower;
            FreeBlock(pHole);
            ++mSwaps;
            pScan->mMemAddr = 0;
            if (nFreed > nBlocks * kVramSwapBudgetMultiple) {
                break;
            }
            pScan = pNextUsed;
        }

        if (nFreed == 0) {
            LogPrintf("!!! VRAM ALLOCATION FAILURE - CALLING CLEAR() !!!\n");
            Clear(0);
            pTaken = mpListHead[kVramListFree];
        }
    }

    pEntry->mMemAddr = pTaken->mMemAddr;
    const unsigned short nTakenSize = pTaken->mSize;
    if (pTaken->mKind != kVramBlockKindFree) {
        // An evicted record stays on the used list, marked non-resident, so its owner uploads
        // again on the next lookup.
        pTaken->mMemAddr = 0;
        ++mSwaps;
    } else {
        mBlocksInUse += nTakenSize;
        UnlinkEntry(pTaken, kVramListFree);
        ReleaseEntry(pTaken);
    }

    if (pTaken->mpLower != nullptr) {
        pTaken->mpLower->mpUpper = pEntry;
    } else {
        mpChainHead = pEntry;
    }
    if (pTaken->mpUpper != nullptr) {
        pTaken->mpUpper->mpLower = pEntry;
    } else {
        mpChainTail = pEntry;
    }
    pEntry->mpUpper = pTaken->mpUpper;
    pEntry->mpLower = pTaken->mpLower;

    if (nBlocks < nTakenSize) {
        VramTableEntry *pRest = AllocEntry();
        const unsigned short nRestSize = static_cast<unsigned short>(nTakenSize - nBlocks);
        pRest->MakeFree(static_cast<unsigned short>(pEntry->mMemAddr + nBlocks), nRestSize);
        if (pEntry->mpUpper != nullptr) {
            pEntry->mpUpper->mpLower = pRest;
        } else {
            mpChainTail = pRest;
        }
        pRest->mpLower = pEntry;
        pRest->mpUpper = pEntry->mpUpper;
        pEntry->mpUpper = pRest;
        FreeBlock(pRest);
        mBlocksInUse -= nRestSize;
    }
}

// 0x00514db8
void VramTable::WipeVram() {
    unsigned char abZero[kVramWipeTileBytes];
    memset(abZero, 0, sizeof(abZero));

    for (int nRow = 0; nRow < kVramWipeRows; nRow += kVramWipeTileTexels) {
        const int nMemAddr = nRow * kVramWipeBlocksPerRow;
        // The inner counter does not affect the transfer. Every pass writes the same tile, and the
        // destination walks past the end of video memory. Both match the binary.
        for (int nColumn = kVramWipeRows - kVramWipeTileTexels; nColumn >= 0;
             nColumn -= kVramWipeTileTexels) {
            LogPrintf("Clearing vram at addr: %d ($%x)\n", nMemAddr, nMemAddr);
            sceGsSetDefLoadImage(&g_vramWipeLoadImage,
                                 static_cast<short>(nMemAddr),
                                 1,
                                 0,
                                 0,
                                 0,
                                 kVramWipeTileTexels,
                                 kVramWipeTileTexels);
            FlushCache(0);
            sceGsExecLoadImage(&g_vramWipeLoadImage, abZero);
        }
    }
}

// 0x00515148
void VramTableEntry::MakeFree(unsigned short nMemAddr, unsigned short nBlocks) {
    mKind = kVramBlockKindFree;
    mMemAddr = nMemAddr;
    mSize = nBlocks;
}

// 0x00514ee8
void VramTableEntry::Reset() {
    mKind = kVramBlockKindNone;
    mMemAddr = 0;
}

// 0x00514ef8
void VramTableEntry::SetPinned(int bPinned) {
    if (bPinned != 0) {
        mLockMask = static_cast<unsigned char>(mLockMask | kVramLockPinned);
    } else {
        mLockMask = static_cast<unsigned char>(mLockMask & ~kVramLockPinned);
    }
}

// 0x00515058
int VramTableEntry::BlocksForImage(int nWidth,
                                   int nHeight,
                                   [[maybe_unused]] int nBitsPerPixel,
                                   int nPsm) const {
    int nPageWidth;
    int nPageHeight;
    switch (nPsm) {
    case kGsPsmT4:
        nPageWidth = 128;
        nPageHeight = 128;
        break;
    case kGsPsmT8:
        nPageWidth = 128;
        nPageHeight = 64;
        break;
    case kGsPsmCt16:
        nPageWidth = 64;
        nPageHeight = 64;
        break;
    default:
        nPageWidth = 64;
        nPageHeight = 32;
        break;
    }
    return ((nWidth + nPageWidth - 1) / nPageWidth) * ((nHeight + nPageHeight - 1) / nPageHeight) *
           kVramPageBlocks;
}

// 0x00514f18
void VramTableEntry::SetupSurface(int nWidth, int nHeight, int nBitsPerPixel, int nPsm, int nKind) {
    const int nBlocks = BlocksForImage(nWidth, nHeight, nBitsPerPixel, nPsm);
    mSize = static_cast<unsigned short>(nBlocks);
    if (nKind == kVramBlockKindRenderTarget) {
        // Room for GetBlockAddr() to round the address up to the next page boundary.
        mSize = static_cast<unsigned short>(nBlocks + kVramPageBlocks - 1);
    }
    mKind = static_cast<unsigned char>(nKind);
    mMemAddr = 0;
    mLockMask = 0;

    VramTableEntry *pHead = g_vramTable.mpListHead[kVramListUsed];
    if (pHead == nullptr) {
        g_vramTable.mpListHead[kVramListUsed] = this;
        g_vramTable.mpListTail[kVramListUsed] = this;
        mpPrev = nullptr;
        mpNext = nullptr;
    } else {
        mpPrev = nullptr;
        mpNext = pHead;
        pHead->mpPrev = this;
        g_vramTable.mpListHead[kVramListUsed] = this;
    }
}

// 0x00513690
int VramTableEntry::GetBlockAddr() {
    if (mLastUsed != g_vramTable.mFlushCount) {
        const int nGeneration = g_vramTable.mLockGeneration - 1;
        const int nBit = 1 << nGeneration;
        if ((mLockMask & nBit) == 0) {
            mLockMask = static_cast<unsigned char>(mLockMask | nBit);
            const int nLocked = g_anVramLockCount[nGeneration];
            g_anVramLockCount[nGeneration] = nLocked + 1;
            g_apVramLocked[nGeneration][nLocked] = this;
        }
    }
    mLastUsed = g_vramTable.mFlushCount;
    if (mMemAddr == 0) {
        ++g_vramTable.mMisses;
    }
    ++g_vramTable.mRequests;

    if (mKind == kVramBlockKindRenderTarget) {
        return (mMemAddr + kVramPageBlocks - 1) & ~(kVramPageBlocks - 1);
    }
    return mMemAddr;
}

// 0x00515160
int VramTableEntry::UploadImage(
    const void *pSource, int nWidth, int nHeight, [[maybe_unused]] int nBitsPerPixel, int nPsm) {
    if (mMemAddr == 0) {
        g_vramTable.AllocBlock(this, mSize);
    }

    if (pSource != nullptr) {
        int nTbw = (nWidth + kGsTexelsPerTbwUnit - 1) / kGsTexelsPerTbwUnit;
        if (nPsm == kGsPsmT8 || nPsm == kGsPsmT4) {
            // An indexed mode needs an even buffer width.
            nTbw = (nTbw + 1) & ~1;
        }
        sceGsSetDefLoadImage(&g_vramUploadLoadImage,
                             static_cast<short>(mMemAddr),
                             static_cast<short>(nTbw),
                             static_cast<short>(nPsm),
                             0,
                             0,
                             static_cast<short>(nWidth),
                             static_cast<short>(nHeight));
        FlushCache(0);
        sceGsExecLoadImage(&g_vramUploadLoadImage, pSource);
    }

    ++g_vramTable.mLoads;
    g_vramTable.mLoadBlocks += mSize;

    if (mKind == kVramBlockKindRenderTarget) {
        return (mMemAddr + kVramPageBlocks - 1) & ~(kVramPageBlocks - 1);
    }
    return mMemAddr;
}

// 0x005152a8
void VramTableEntry::UploadSubImage(const void *pSource,
                                    int nWidth,
                                    int nHeight,
                                    [[maybe_unused]] int nBitsPerPixel,
                                    int nPsm,
                                    int nBlockOffset) {
    int nTbw = (nWidth + kGsTexelsPerTbwUnit - 1) / kGsTexelsPerTbwUnit;
    if (nPsm == kGsPsmT8 || nPsm == kGsPsmT4) {
        nTbw = (nTbw + 1) & ~1;
    }
    sceGsSetDefLoadImage(&g_vramUploadSubLoadImage,
                         static_cast<short>(mMemAddr + nBlockOffset),
                         static_cast<short>(nTbw),
                         static_cast<short>(nPsm),
                         0,
                         0,
                         static_cast<short>(nWidth),
                         static_cast<short>(nHeight));
    FlushCache(0);
    sceGsExecLoadImage(&g_vramUploadSubLoadImage, pSource);
}

// 0x00514fa8
void VramTableEntry::FreeSelf() {
    mLockMask = 0;
    g_vramTable.UnlinkEntry(this, kVramListUsed);
    if (mMemAddr != 0) {
        g_vramTable.mBlocksInUse -= mSize;
        g_vramTable.FreeBlock(this);
    } else {
        g_vramTable.ReleaseEntry(this);
    }
}

// 0x00513900
int VramPalEntry::AllocBlock() {
    int nSlot = 0;
    if (g_adVramPalSlots[0] == 0xffffffffu) {
        for (int nWord = 1; nWord < kVramPalSlotWords; ++nWord) {
            if (g_adVramPalSlots[nWord] != 0xffffffffu) {
                nSlot = nWord * kVramPalSlotsPerWord;
                break;
            }
        }
    }
    // Every shift below takes the slot index unmasked, so only its low five bits select the bit.
    while (nSlot < kVramPalSlots &&
           (g_adVramPalSlots[nSlot / kVramPalSlotsPerWord] & (1u << nSlot)) != 0) {
        ++nSlot;
    }
    if (nSlot >= kVramPalSlots) {
        nSlot = SwapOutOldest();
    }
    if ((g_adVramPalSlots[nSlot / kVramPalSlotsPerWord] & (1u << nSlot)) != 0) {
        Fatal("AllocBlock: trying to set vpalIndex: %d, already set\n", nSlot);
    }
    g_adVramPalSlots[nSlot / kVramPalSlotsPerWord] |= 1u << nSlot;
    return g_vramTable.mPaletteBase + nSlot * kVramPalBlocks;
}

// 0x00513780
int VramPalEntry::SwapOutOldest() {
    // The head of the resident chain is dereferenced without a null test.
    VramPalEntry *pPal = g_pVramPalUsed;
    while (pPal->mpNext != nullptr) {
        pPal = pPal->mpNext;
    }

    int nSlot = 0;
    int nReleased = 0;
    unsigned short nLastAddr = 0;
    while (pPal != nullptr) {
        if (pPal->mMemAddr == 0) {
            Fatal("Pal entry in chain has mMemAddr of 0 (mpPrev: %p, mpNext: %p)\n",
                  pPal->mpPrev,
                  pPal->mpNext);
        }
        VramPalEntry *pNewer = pPal->mpPrev;
        if (pPal->mLockMask == 0) {
            if (pPal->mpNext != nullptr) {
                pPal->mpNext->mpPrev = pPal->mpPrev;
            }
            if (pPal->mpPrev != nullptr) {
                pPal->mpPrev->mpNext = pPal->mpNext;
            } else {
                g_pVramPalUsed = pPal->mpNext;
            }

            nLastAddr = pPal->mMemAddr;
            ++nReleased;
            nSlot = (nLastAddr - g_vramTable.mPaletteBase) / kVramPalBlocks;
            g_adVramPalSlots[nSlot / kVramPalSlotsPerWord] &= ~(1u << nSlot);
            pPal->mMemAddr = 0;
            if (nReleased >= kVramPalSwapOutLimit) {
                break;
            }
        }
        pPal = pNewer;
    }

    if (nLastAddr == 0) {
        Fatal("No unlocked VRAM pal entries to swap out!\n");
    }
    return nSlot;
}

// 0x005153f0
int VramPalEntry::GetSlotIndex() const {
    return (mMemAddr - g_vramTable.mPaletteBase) / kVramPalBlocks;
}

// 0x005154c8
void VramPalEntry::ClearLockMask() {
    mLockMask = 0;
}

// 0x00513a40
int VramPalEntry::GetBlockAddr() {
    const int bStale = (mLastUsed != g_vramTable.mFlushCount) ? 1 : 0;
    const int nGeneration = g_vramTable.mLockGeneration - 1;
    // The second test reads the block lock count rather than the palette one. Yes, the binary
    // reads the wrong array here.
    if (bStale != 0 || g_anVramLockCount[nGeneration] == 0) {
        const int nBit = 1 << nGeneration;
        if ((mLockMask & nBit) == 0) {
            mLockMask = static_cast<unsigned short>(mLockMask | nBit);
            const int nLocked = g_anVramPalLockCount[nGeneration];
            g_anVramPalLockCount[nGeneration] = nLocked + 1;
            g_apVramPalLocked[nGeneration][nLocked] = this;
        }
    }
    mLastUsed = g_vramTable.mFlushCount;

    if (bStale != 0 && mMemAddr != 0) {
        if (mpNext != nullptr) {
            mpNext->mpPrev = mpPrev;
        }
        if (mpPrev != nullptr) {
            mpPrev->mpNext = mpNext;
        } else {
            g_pVramPalUsed = mpNext;
        }
        mpNext = g_pVramPalUsed;
        if (g_pVramPalUsed != nullptr) {
            g_pVramPalUsed->mpPrev = this;
        }
        mpPrev = nullptr;
        g_pVramPalUsed = this;
    }
    return mMemAddr;
}

// 0x00515590
int VramPalEntry::UploadClut(
    const void *pSource, int nWidth, int nHeight, [[maybe_unused]] int nBitsPerPixel, int nPsm) {
    if (mMemAddr == 0) {
        mMemAddr = static_cast<unsigned short>(AllocBlock());
        if (g_pVramPalUsed != nullptr) {
            g_pVramPalUsed->mpPrev = this;
        }
        mpPrev = nullptr;
        mpNext = g_pVramPalUsed;
        g_pVramPalUsed = this;
    }

    sceGsSetDefLoadImage(&g_vramPalLoadImage,
                         static_cast<short>(mMemAddr),
                         kVramPalTbw,
                         static_cast<short>(nPsm),
                         0,
                         0,
                         static_cast<short>(nWidth),
                         static_cast<short>(nHeight));
    FlushCache(0);
    sceGsExecLoadImage(&g_vramPalLoadImage, pSource);
    return mMemAddr;
}

// 0x005154d0
void VramPalEntry::FreeSelf() {
    if (mMemAddr != 0) {
        if (mpPrev != nullptr) {
            mpPrev->mpNext = mpNext;
        } else {
            g_pVramPalUsed = mpNext;
        }
        if (mpNext != nullptr) {
            mpNext->mpPrev = mpPrev;
        }
        const int nSlot = (mMemAddr - g_vramTable.mPaletteBase) / kVramPalBlocks;
        g_adVramPalSlots[nSlot / kVramPalSlotsPerWord] &= ~(1u << nSlot);
        mMemAddr = 0;
    }
    mLockMask = 0;
    mpNext = g_pVramPalFree;
    g_pVramPalFree = this;
}
