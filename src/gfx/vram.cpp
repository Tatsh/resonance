#include "gfx/vram.h"

#include <eekernel.h>
#include <libgraph.h>
#include <stdio.h>
#include <string.h>

#include "gfx/gfxdevice.h"
#include "os/log.h"
#include "os/zone.h"
#include "rndartt/abitmap.h"
#include "rndartt/agfxfile.h"

// NTSC-U/C: 0x0070d3d0, PAL: 0x007512c0
const int g_anGsPixelStorageModes[kABitmapFormatCount] = {
    kGsPsmT4, kGsPsmT8, kGsPsmCt16, kGsPsmCt24, kGsPsmCt32, kGsPsmT8};

// NTSC-U/C: 0x0070d3e8, PAL: 0x007512d8
const int g_anBitsPerPixelTable[kABitmapFormatCount] = {4, 8, 16, 24, 32, 8};

// NTSC-U/C: 0x0089dd90, PAL: 0x008e2dd0
sceGsStoreImage g_vramReadBackStoreImage;

// NTSC-U/C: 0x0089de00, PAL: 0x008e2e40
sceGsLoadImage g_vramWipeLoadImage;

// NTSC-U/C: 0x00718468, PAL: 0x0075c358
int g_nScreendumpIndex = 1;

namespace {

// The zone Screendump() allocates its pixels from, and the file name it formats.
constexpr char kScreendumpZoneName[] = "temp";
constexpr char kScreendumpPathFormat[] = "%s_%d.bmp";

// Bytes per pixel of the kABitmapFormatLinear15 bitmap Screendump() captures into.
constexpr int kScreendumpBytesPerPixel = 2;

// Size of the buffer Screendump() formats the file name into.
constexpr int kScreendumpPathSize = 128;

// Arguments to sceGsSyncPath() that wait for every path without a timeout.
constexpr int kGsSyncPathWait = 0;
constexpr unsigned short kGsSyncPathNoTimeout = 0;

// The write-back mode of FlushCache().
constexpr int kFlushCacheWriteBackData = 0;

} // namespace

Rnd::VRAM::~VRAM() {
}

void Rnd::VRAM::Init() {
    mFlushCount = 0;
    mPaletteBase = Rnd::ThePs.GetReservedVramWords() / (kVramBlockBytes / 4);
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

void Rnd::VRAM::Clear(int bClearPalettes) {
    Entry *pEntry = mpListHead[kVramListFree];
    while (pEntry != nullptr) {
        Entry *pNext = pEntry->mpNext;
        ReleaseEntry(pEntry);
        pEntry = pNext;
    }

    for (pEntry = mpListHead[kVramListUsed]; pEntry != nullptr; pEntry = pEntry->mpNext) {
        if ((pEntry->mLockMask & kVramLockGenerationMask) != 0) {
            printf("%s() - resetting lock on entry %d\n",
                   __func__,
                   static_cast<int>(pEntry - g_vramEntries));
        }
        pEntry->mMemAddr = 0;
        pEntry->mLockMask =
            static_cast<unsigned char>(pEntry->mLockMask & ~kVramLockGenerationMask);
    }

    Entry *pWhole = GetAvailEntry();
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

void Rnd::VRAM::BeginFrame() {
}

void Rnd::VRAM::EndFrame() {
    for (Entry *pEntry = mpListHead[kVramListUsed]; pEntry != nullptr; pEntry = pEntry->mpNext) {
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

void Rnd::VRAM::AdvanceLockCycle() {
    // The counter is advanced through the file scope instance rather than through this.
    ++Rnd::TheVRAM.mFlushCount;

    mLockGeneration = static_cast<unsigned char>(mLockGeneration + 1);
    if (mLockGeneration > kVramLockGenerations) {
        mLockGeneration = 1;
    }

    const int nGeneration = mLockGeneration - 1;
    const int nBit = 1 << nGeneration;

    Entry **ppEntry = g_apVramLocked[nGeneration];
    for (int nLocked = g_anVramLockCount[nGeneration]; nLocked > 0; --nLocked) {
        if (((*ppEntry)->mLockMask & nBit) == 0) {
            printf("VRAM should have lockmask %d set but instead has %d\n",
                   nBit,
                   (*ppEntry)->mLockMask);
        }
        (*ppEntry)->mLockMask = static_cast<unsigned char>((*ppEntry)->mLockMask & ~nBit);
        ++ppEntry;
    }

    VramPalEntry **ppPal = g_apVramPalLocked[nGeneration];
    for (int nPalLocked = g_anVramPalLockCount[nGeneration]; nPalLocked > 0; --nPalLocked) {
        if (((*ppPal)->mLockMask & nBit) == 0) {
            printf("VRAM pal entry should have lockmask %d set but instead has %d\n",
                   nBit,
                   (*ppPal)->mLockMask);
        }
        (*ppPal)->mLockMask = static_cast<unsigned short>((*ppPal)->mLockMask & ~nBit);
        ++ppPal;
    }

    g_anVramLockCount[nGeneration] = 0;
    g_anVramPalLockCount[nGeneration] = 0;
}

void Rnd::VRAM::PrintStats(const char *pszPrefix) {
    printf("%sVRAM in use: %d blocks (%d bytes)\n",
           pszPrefix,
           mBlocksInUse,
           mBlocksInUse * kVramBlockBytes);

    const int nRequests = (mRequests != 0) ? mRequests : 1;
    const int nMissPercent =
        static_cast<int>(static_cast<float>(mMisses) * 100.0f / static_cast<float>(nRequests));
    const int nSwapPercent =
        static_cast<int>(static_cast<float>(mSwaps) * 100.0f / static_cast<float>(nRequests));
    printf("%sVRAM stats (frame):  loads: %d (%d blocks), misses: %d (%d%%), "
           "swaps: %d (%d%%), freemerge: %d\n",
           pszPrefix,
           mLoads,
           mLoadBlocks,
           mMisses,
           nMissPercent,
           mSwaps,
           nSwapPercent,
           mFreeMerges);
    printf("%sVRAM stats (accum):  loads: %d (%d blocks), misses: %d, swaps: %d, "
           "freemerge: %d\n",
           pszPrefix,
           mAccumLoads,
           mAccumLoadBlocks,
           mAccumMisses,
           mAccumSwaps,
           mAccumFreeMerges);
}

void Rnd::VRAM::GetLastFrameLoads(int *pnLoads, int *pnBlocks) const {
    *pnLoads = g_nVramLoadsLastFrame;
    *pnBlocks = g_nVramLoadBlocksLastFrame;
}

Rnd::VRAM::Entry *Rnd::VRAM::GetAvailEntry() {
    Entry *pEntry = mpListHead[kVramListPool];
    if (pEntry == nullptr) {
        printf("Out of VRAM Table Entries!!!\n");
        return nullptr;
    }

    Entry *pNext = pEntry->mpNext;
    mpListHead[kVramListPool] = pNext;
    if (pNext != nullptr) {
        pNext->mpPrev = nullptr;
    }
    if (mpListTail[kVramListPool] == pEntry) {
        mpListTail[kVramListPool] = nullptr;
    }
    return pEntry;
}

VramPalEntry *Rnd::VRAM::AllocPalEntry() {
    VramPalEntry *pPal = g_pVramPalFree;
    if (pPal == nullptr) {
        Fatal("Exceeded %d available VRAM palette entries!\n", kVramPalEntries);
    }
    g_pVramPalFree = pPal->mpNext;
    return pPal;
}

void Rnd::VRAM::UnlinkEntry(Entry *pEntry, int nList) {
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

void Rnd::VRAM::ReleaseEntry(Entry *pEntry) {
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

void Rnd::VRAM::MergeBlocks(Entry *pKeep, Entry *pAbsorb) {
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

void Rnd::VRAM::FreeBlock(Entry *pEntry) {
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

    Entry *pUpper = pEntry->mpUpper;
    if (pUpper != nullptr && pUpper->mKind == kVramBlockKindFree) {
        MergeBlocks(pEntry, pUpper);
    }
    Entry *pLower = pEntry->mpLower;
    if (pLower != nullptr && pLower->mKind == kVramBlockKindFree) {
        MergeBlocks(pLower, pEntry);
    }
}

void Rnd::VRAM::RemoveFromChain(Entry *pEntry) {
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

    Entry *pLower = pEntry->mpLower;
    Entry *pUpper = pEntry->mpUpper;
    if (pLower != nullptr && pUpper != nullptr && pLower->mKind == kVramBlockKindFree &&
        pUpper->mKind == pLower->mKind) {
        MergeBlocks(pLower, pUpper);
    }
}

void Rnd::VRAM::AllocBlock(Entry *pEntry, unsigned short nBlocks) {
    Entry *pTaken = nullptr;

    while (pTaken == nullptr) {
        int nBestSize = 0;
        unsigned int nBestAge = 0;

        for (Entry *pFree = mpListHead[kVramListFree]; pFree != nullptr; pFree = pFree->mpNext) {
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

        Entry *pUsed = mpListHead[kVramListUsed];
        for (Entry *pScan = pUsed; pScan != nullptr; pScan = pScan->mpNext) {
            if (pScan->mLockMask != 0 || pScan->mMemAddr == 0) {
                continue;
            }
            // Yes, the binary reads the global table's counter here rather than this table's.
            const unsigned int nAge =
                static_cast<unsigned int>(Rnd::TheVRAM.mFlushCount - pScan->mLastUsed);
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
        Entry *pScan = pUsed;
        while (pScan != nullptr) {
            if (pScan->mLockMask != 0) {
                pScan = pScan->mpNext;
                continue;
            }
            Entry *pNextUsed = pScan->mpNext;
            if (pScan->mMemAddr == 0) {
                pScan = pNextUsed;
                continue;
            }

            nFreed += pScan->mSize;
            // A null record here is dereferenced by MakeFree(). Yes, the binary does that.
            Entry *pHole = GetAvailEntry();
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
            printf("!!! VRAM ALLOCATION FAILURE - CALLING CLEAR() !!!\n");
            Clear(0);
            pTaken = mpListHead[kVramListFree];
            break; // Yes, the binary does not test the reloaded free head.
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
        Entry *pRest = GetAvailEntry();
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

void Rnd::VRAM::ReadBackBitmap(ABitmap *pBitmap, unsigned short nMemAddr) {
    const int nBufferWidth = (pBitmap->mWidth + kGsTexelsPerTbwUnit - 1) / kGsTexelsPerTbwUnit;
    sceGsSetDefStoreImage(&g_vramReadBackStoreImage,
                          static_cast<short>(nMemAddr),
                          static_cast<short>(nBufferWidth),
                          static_cast<short>(g_anGsPixelStorageModes[pBitmap->mFormat]),
                          0,
                          0,
                          pBitmap->mWidth,
                          pBitmap->mHeight);
    FlushCache(kFlushCacheWriteBackData);
    sceGsSyncPath(kGsSyncPathWait, kGsSyncPathNoTimeout);
    sceGsExecStoreImage(&g_vramReadBackStoreImage, pBitmap->mPixels);
    sceGsSyncPath(kGsSyncPathWait, kGsSyncPathNoTimeout);
}

void Rnd::VRAM::Screendump(const char *pszName) {
    const int nPreviousZone = ZoneGetCurrent();
    ZoneSetCurrent(FindZoneByName(kScreendumpZoneName));
    ZoneReset();
    ABitmap bitmap(ZoneAlloc(Rnd::ThePs.mnDisplayWidth * Rnd::ThePs.mnDisplayHeight *
                             kScreendumpBytesPerPixel),
                   kABitmapFormatLinear15,
                   false,
                   Rnd::ThePs.mnDisplayWidth,
                   Rnd::ThePs.mnDisplayHeight,
                   0);
    ZoneSetCurrent(nPreviousZone);

    ReadBackBitmap(&bitmap, 0);
    bitmap.SwapRedBlue();

    char szPath[kScreendumpPathSize];
    sprintf(szPath, kScreendumpPathFormat, pszName, g_nScreendumpIndex++);
    AGfxFile::Write(szPath, bitmap);
}

void Rnd::VRAM::WipeVram() {
    unsigned char abZero[kVramWipeTileBytes];
    memset(abZero, 0, sizeof(abZero));

    for (int nRow = 0; nRow < kVramWipeRows; nRow += kVramWipeTileTexels) {
        const int nMemAddr = nRow * kVramWipeBlocksPerRow;
        // The inner counter does not affect the transfer. Every pass writes the same tile, and the
        // destination walks past the end of video memory. Both match the binary.
        for (int nColumn = kVramWipeRows - kVramWipeTileTexels; nColumn >= 0;
             nColumn -= kVramWipeTileTexels) {
            printf("Clearing vram at addr: %d ($%x)\n", nMemAddr, nMemAddr);
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

void Rnd::VRAM::Entry::MakeFree(unsigned short nMemAddr, unsigned short nBlocks) {
    mKind = kVramBlockKindFree;
    mMemAddr = nMemAddr;
    mSize = nBlocks;
}

void Rnd::VRAM::Entry::Reset() {
    mKind = kVramBlockKindNone;
    mMemAddr = 0;
}

void Rnd::VRAM::Entry::SetPinned(int bPinned) {
    if (bPinned != 0) {
        mLockMask = static_cast<unsigned char>(mLockMask | kVramLockPinned);
    } else {
        mLockMask = static_cast<unsigned char>(mLockMask & ~kVramLockPinned);
    }
}

int Rnd::VRAM::Entry::ComputeBlockSize(int nWidth,
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

void Rnd::VRAM::Entry::SetupSurface(
    int nWidth, int nHeight, int nBitsPerPixel, int nPsm, int nKind) {
    const int nBlocks = ComputeBlockSize(nWidth, nHeight, nBitsPerPixel, nPsm);
    mSize = static_cast<unsigned short>(nBlocks);
    if (nKind == kVramBlockKindRenderTarget) {
        // Room for GetBlockAddr() to round the address up to the next page boundary.
        mSize = static_cast<unsigned short>(nBlocks + kVramPageBlocks - 1);
    }
    mKind = static_cast<unsigned char>(nKind);
    mMemAddr = 0;
    mLockMask = 0;

    Entry *pHead = Rnd::TheVRAM.mpListHead[kVramListUsed];
    if (pHead == nullptr) {
        Rnd::TheVRAM.mpListHead[kVramListUsed] = this;
        Rnd::TheVRAM.mpListTail[kVramListUsed] = this;
        mpPrev = nullptr;
        mpNext = nullptr;
    } else {
        mpPrev = nullptr;
        mpNext = pHead;
        pHead->mpPrev = this;
        Rnd::TheVRAM.mpListHead[kVramListUsed] = this;
    }
}

int Rnd::VRAM::Entry::GetBlockAddr() {
    if (mLastUsed != Rnd::TheVRAM.mFlushCount) {
        const int nGeneration = Rnd::TheVRAM.mLockGeneration - 1;
        const int nBit = 1 << nGeneration;
        if ((mLockMask & nBit) == 0) {
            mLockMask = static_cast<unsigned char>(mLockMask | nBit);
            const int nLocked = g_anVramLockCount[nGeneration];
            g_anVramLockCount[nGeneration] = nLocked + 1;
            g_apVramLocked[nGeneration][nLocked] = this;
        }
    }
    mLastUsed = Rnd::TheVRAM.mFlushCount;
    if (mMemAddr == 0) {
        ++Rnd::TheVRAM.mMisses;
    }
    ++Rnd::TheVRAM.mRequests;

    if (mKind == kVramBlockKindRenderTarget) {
        return (mMemAddr + kVramPageBlocks - 1) & ~(kVramPageBlocks - 1);
    }
    return mMemAddr;
}

int Rnd::VRAM::Entry::UploadImage(
    const void *pSource, int nWidth, int nHeight, [[maybe_unused]] int nBitsPerPixel, int nPsm) {
    if (mMemAddr == 0) {
        Rnd::TheVRAM.AllocBlock(this, mSize);
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

    ++Rnd::TheVRAM.mLoads;
    Rnd::TheVRAM.mLoadBlocks += mSize;

    if (mKind == kVramBlockKindRenderTarget) {
        return (mMemAddr + kVramPageBlocks - 1) & ~(kVramPageBlocks - 1);
    }
    return mMemAddr;
}

void Rnd::VRAM::Entry::UploadSubImage(const void *pSource,
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

void Rnd::VRAM::Entry::FreeSelf() {
    mLockMask = 0;
    Rnd::TheVRAM.UnlinkEntry(this, kVramListUsed);
    if (mMemAddr != 0) {
        Rnd::TheVRAM.mBlocksInUse -= mSize;
        Rnd::TheVRAM.FreeBlock(this);
    } else {
        Rnd::TheVRAM.ReleaseEntry(this);
    }
}

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
    return Rnd::TheVRAM.mPaletteBase + nSlot * kVramPalBlocks;
}

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
            nSlot = (nLastAddr - Rnd::TheVRAM.mPaletteBase) / kVramPalBlocks;
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

int VramPalEntry::GetSlotIndex() const {
    return (mMemAddr - Rnd::TheVRAM.mPaletteBase) / kVramPalBlocks;
}

void VramPalEntry::ClearLockMask() {
    mLockMask = 0;
}

int VramPalEntry::GetBlockAddr() {
    const int bStale = (mLastUsed != Rnd::TheVRAM.mFlushCount) ? 1 : 0;
    const int nGeneration = Rnd::TheVRAM.mLockGeneration - 1;
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
    mLastUsed = Rnd::TheVRAM.mFlushCount;

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
        const int nSlot = (mMemAddr - Rnd::TheVRAM.mPaletteBase) / kVramPalBlocks;
        g_adVramPalSlots[nSlot / kVramPalSlotsPerWord] &= ~(1u << nSlot);
        mMemAddr = 0;
    }
    mLockMask = 0;
    mpNext = g_pVramPalFree;
    g_pVramPalFree = this;
}

Rnd::VRAM Rnd::TheVRAM;
Rnd::VRAM::Entry g_vramEntries[kVramTableEntries];
VramPalEntry g_vramPalEntries[kVramPalEntries];
Rnd::VRAM::Entry *g_apVramLocked[kVramLockGenerations][kVramLockedPerGeneration];
VramPalEntry *g_apVramPalLocked[kVramLockGenerations][kVramLockedPerGeneration];
VramPalEntry *g_pVramPalFree;
VramPalEntry *g_pVramPalUsed;
int g_anVramLockCount[kVramLockGenerations];
int g_anVramPalLockCount[kVramLockGenerations];
unsigned g_adVramPalSlots[kVramPalSlotWords];
sceGsLoadImage g_vramPalLoadImage;
sceGsLoadImage g_vramUploadLoadImage;
sceGsLoadImage g_vramUploadSubLoadImage;
int g_nVramLoadsLastFrame;
int g_nVramLoadBlocksLastFrame;
