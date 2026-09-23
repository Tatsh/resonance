#include "rnd/pstex.h"

#include <string.h>

#include "gfx/gfxdevice.h"
#include "gfx/vramtable.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "os/zone.h"
#include "rndartt/abitmap.h"
#include "rndartt/acanvas.h"
#include "rndartt/apalette.h"
#include "rndartt/arlereader.h"

namespace Rnd {

namespace {

// GS registers the texture programs.
constexpr int kGsRegTex0 = 0x06;
constexpr int kGsRegTex1 = 0x14;
constexpr int kGsRegMipTbp1 = 0x34;
constexpr int kGsRegMipTbp2 = 0x36;
constexpr unsigned long long kGsRegAllBits = ~0ULL;

// A base page address is fourteen bits wherever it appears. TEX0 has TBP0 at bit 0 and CBP at bit
// 37, and each MIPTBP register has three at bits 0, 20, and 40.
constexpr unsigned long long kGsTbpMask = 0x3fff;
constexpr int kTex0CbpShift = 37;
constexpr int kMipTbpFirstShift = 0;
constexpr int kMipTbpSecondShift = 20;
constexpr int kMipTbpThirdShift = 40;

// The registers BindAsRenderTarget() programs, the TEX0 fields it copies into FRAME_1, and the
// fields of each register it sets.
constexpr int kGsRegXyOffset1 = 0x18;
constexpr int kGsRegTest1 = 0x47;
constexpr int kGsRegFrame1 = 0x4c;
constexpr int kGsRegZbuf1 = 0x4e;
constexpr int kTex0TbwShift = 14;
constexpr int kTex0PsmShift = 20;
constexpr unsigned long long kSixBitField = 0x3f;
constexpr int kFrameFbwShift = 16;
constexpr int kFramePsmShift = 24;
constexpr unsigned long long kFrameFbpMask = 0x1ff;
constexpr unsigned long long kFrameFields = 0x3f3f01ff;
constexpr int kBlocksPerFramePageShift = 5;
constexpr unsigned long long kXyOffsetFields = 0x0000ffff0000ffffULL;
constexpr int kXyOffsetYShift = 32;
constexpr int kGsCoordinateCentre = 0x800;
constexpr int kGsSubpixelShift = 4;
constexpr unsigned long long kGsSubpixelCoordMask = 0xffff;
constexpr unsigned long long kZbufZmsk = 1ULL << 32;
constexpr unsigned long long kTestZtstAlways = 1ULL << 17;
constexpr unsigned long long kTestZtstMask = 3ULL << 17;

// TEX0.TFX, two bits at bit 35.
constexpr unsigned long long kTex0TfxMask = 3;
constexpr int kTex0TfxShift = 35;

// The TEX1 fields StaticInit() programs, MXL bit 2 through MMIN, and the value it writes.
constexpr unsigned long long kTex1DefaultFilter = 0x30;
constexpr unsigned long long kTex1FilterMask = 0x1f0;

// The sign bit of mDirtyMips stands for the CLUT.
constexpr unsigned kDirtyClut = 0x80000000u;

// Bit of LockMipBitmap()'s flags that requests the read-back.
constexpr int kLockReadBack = 2;

// Shapes and format of a CLUT upload. A four-bit level has 16 entries, every other 256.
constexpr int kClut16Width = 8;
constexpr int kClut16Height = 2;
constexpr int kClut256Width = 16;
constexpr int kClut256Height = 16;
constexpr int kClutBitsPerEntry = 32;
constexpr int kGsPsmCt32 = 0;

// A 256-entry CLUT moves in 32 groups of eight entries.
constexpr int kClutGroupCount = 32;
constexpr int kClutGroupEntries = 8;
constexpr int kClutSwizzleSpan = 4;

// A decompressed run-length level is uploaded as eight-bit indices.
constexpr int kRleBitsPerPixel = 8;

// Levels whose base page the three register images record.
enum MipTbpLevel {
    kMipTbpLevelTex0 = 0,
    kMipTbpLevel1 = 1,
    kMipTbpLevel2 = 2,
    kMipTbpLevel3 = 3,
    kMipTbpLevel4 = 4,
    kMipTbpLevel5 = 5,
    kMipTbpLevel6 = 6
};

} // namespace

// 0x0076f368
const int g_anClutSwizzleBlocks[4] = {0, 2, 1, 3};

// 0x0076f378
const int g_anPackedMipPageOffsets[6] = {16, 20, 21, 22, 23, 24};

// 0x00596f80
PsTex::PsTex(const HxStr &name) : Tex(name), mPaletteVram(nullptr) {
}

// 0x0059a558
PsTex::~PsTex() {
    FreeGsSurfaces();
}

// 0x00596fd8
void PsTex::OnMipLoaded(int nMip) {
    Tex::OnMipLoaded(nMip);
    if (mLoadedBitmaps.empty()) {
        return;
    }
    ABitmap *pBitmap = mLoadedBitmaps[nMip];
    if (pBitmap == nullptr) {
        return;
    }

    APalette *pPalette = pBitmap->mPalette;
    if (pPalette != nullptr) {
        for (int nEntry = 0; nEntry < pPalette->mEnd; ++nEntry) {
            pPalette->mEntries[nEntry] = HalveAlpha(pPalette->mEntries[nEntry]);
        }
        return;
    }
    if (pBitmap->mFormat != kABitmapFormatLinear32) {
        return;
    }
    for (int nRow = 0; nRow < pBitmap->mHeight; ++nRow) {
        unsigned int *pTexel = reinterpret_cast<unsigned int *>(
            static_cast<unsigned char *>(pBitmap->mPixels) + nRow * pBitmap->mBytesPerRow);
        for (int nColumn = 0; nColumn < pBitmap->mWidth; ++nColumn) {
            pTexel[nColumn] = HalveAlpha(pTexel[nColumn]);
        }
    }
}

// 0x00597130
void PsTex::FreeGsSurfaces() {
    if (mPaletteVram != nullptr) {
        mPaletteVram->FreeSelf();
        mPaletteVram = nullptr;
    }
    for (auto it = mGsMips.begin(); it != mGsMips.end(); ++it) {
        if (it->mPage != nullptr) {
            it->mPage->FreeSelf();
            it->mPage = nullptr;
        }
        if (it->mVramBitmap != nullptr) {
            delete it->mVramBitmap;
            it->mVramBitmap = nullptr;
        }
    }
    mGsMips.clear();
}

// 0x00597c68
void PsTex::RebuildClut() {
    if (mLoadedBitmaps.empty()) {
        return;
    }
    const ABitmap *pBitmap = mLoadedBitmaps[0];
    if (pBitmap == nullptr) {
        return;
    }

    const APalette *pPalette = pBitmap->mPalette;
    if (pBitmap->mFormat == kABitmapFormatLinear4) {
        mClut.SetEntries(pPalette->mEntries, 0, pPalette->mEnd);
        return;
    }
    for (int nGroup = 0; nGroup < kClutGroupCount; ++nGroup) {
        const int nSource = (nGroup & ~(kClutSwizzleSpan - 1)) +
                            g_anClutSwizzleBlocks[nGroup & (kClutSwizzleSpan - 1)];
        memcpy(&mClut.mEntries[nGroup * kClutGroupEntries],
               &pPalette->mEntries[nSource * kClutGroupEntries],
               sizeof(unsigned int) * kClutGroupEntries);
    }
}

// 0x00597d50
int PsTex::UploadBitmapMipToGs(int nMip) {
    const ABitmap *pBitmap = mLoadedBitmaps[nMip];
    GsMip &mip = mGsMips[nMip];

    if (pBitmap->mFormat != kABitmapFormatRle8) {
        return mip.mPage->UploadImage(
            pBitmap->mPixels, pBitmap->mWidth, pBitmap->mHeight, mBitsPerPixel, mGsPsm);
    }

    unsigned char *pIndices =
        static_cast<unsigned char *>(ZoneGrabTemp(pBitmap->mWidth * pBitmap->mHeight));
    ARleReader reader;
    reader.mSource = static_cast<const unsigned char *>(pBitmap->mPixels);
    reader.mWidth = pBitmap->mWidth;
    reader.mTransparentValue = pBitmap->mHasTransparentColor ?
                                   static_cast<int>(pBitmap->mTransparentColor) :
                                   kARleReaderNoTransparentValue;
    reader.DecodeRows(pIndices);
    ZoneReleaseTemp(); // Yes, the binary releases the buffer before the upload reads it.
    return mip.mPage->UploadImage(
        pIndices, pBitmap->mWidth, pBitmap->mHeight, kRleBitsPerPixel, kGsPsmT8);
}

// 0x0059ac68
inline int PsTex::UploadPaletteClut() {
    int nWidth = kClut256Width;
    int nHeight = kClut256Height;
    if (mLoadedBitmaps[0]->mFormat == kABitmapFormatLinear4) {
        nWidth = kClut16Width;
        nHeight = kClut16Height;
    }
    return mPaletteVram->UploadClut(mClut.mEntries, nWidth, nHeight, kClutBitsPerEntry, kGsPsmCt32);
}

// 0x0059acc0
inline void PsTex::UploadBitmapMipToSubImage(VramTableEntry *pPage, int nMip, int nBlockOffset) {
    const ABitmap *pBitmap = mLoadedBitmaps[nMip];

    if (pBitmap->mFormat != kABitmapFormatRle8) {
        pPage->UploadSubImage(pBitmap->mPixels,
                              pBitmap->mWidth,
                              pBitmap->mHeight,
                              mBitsPerPixel,
                              mGsPsm,
                              nBlockOffset);
        return;
    }

    unsigned char *pIndices =
        static_cast<unsigned char *>(ZoneGrabTemp(pBitmap->mWidth * pBitmap->mHeight));
    ARleReader reader;
    reader.mSource = static_cast<const unsigned char *>(pBitmap->mPixels);
    reader.mWidth = pBitmap->mWidth;
    reader.mTransparentValue = pBitmap->mHasTransparentColor ?
                                   static_cast<int>(pBitmap->mTransparentColor) :
                                   kARleReaderNoTransparentValue;
    reader.DecodeRows(pIndices);
    ZoneReleaseTemp(); // Yes, the binary releases the buffer before the upload reads it.
    pPage->UploadSubImage(
        pIndices, pBitmap->mWidth, pBitmap->mHeight, kRleBitsPerPixel, kGsPsmT8, nBlockOffset);
}

// 0x00597e38
void PsTex::UploadPendingMips() {
    if (mDirtyMips == 0) {
        return;
    }
    if (mLoadedBitmaps.empty() || mLoadedBitmaps[0] == nullptr) {
        return;
    }

    if ((mDirtyMips & kDirtyClut) != 0) {
        if (mPaletteVram != nullptr) {
            const unsigned long long qwCbp =
                static_cast<unsigned long long>(UploadPaletteClut()) & kGsTbpMask;
            mTex0 = (mTex0 & ~(kGsTbpMask << kTex0CbpShift)) | (qwCbp << kTex0CbpShift);
        } else {
            LogPrintf("Dirty Palette bit, but no pPaletteVram.. rgba %p\n", &mClut);
        }
    } else if (mPaletteVram != nullptr) {
        unsigned long long qwCbp =
            static_cast<unsigned long long>(mPaletteVram->GetBlockAddr()) & kGsTbpMask;
        mTex0 = (mTex0 & ~(kGsTbpMask << kTex0CbpShift)) | (qwCbp << kTex0CbpShift);
        if ((mTex0 & (kGsTbpMask << kTex0CbpShift)) == 0) {
            qwCbp = static_cast<unsigned long long>(UploadPaletteClut()) & kGsTbpMask;
            mTex0 = (mTex0 & ~(kGsTbpMask << kTex0CbpShift)) | (qwCbp << kTex0CbpShift);
        }
    }

    for (unsigned nMip = 0; nMip < mLoadedBitmaps.size(); ++nMip) {
        if ((mDirtyMips & (1u << nMip)) != 0) {
            UploadMipAndBuildMipTbp(static_cast<int>(nMip), false);
        }
    }
    mDirtyMips = 0;
}

// 0x00598000
bool PsTex::BindToGsSlot(unsigned nTexFunc) {
    WaitForMipsLoaded();
    if (mLoadedBitmaps.empty() || mLoadedBitmaps[0] == nullptr) {
        return false;
    }
    if (mGsMips.empty()) {
        return false;
    }

    UploadPendingMips();
    mTex0 = (mTex0 & ~(kTex0TfxMask << kTex0TfxShift)) |
            (static_cast<unsigned long long>(nTexFunc & kTex0TfxMask) << kTex0TfxShift);

    if (mPaletteVram != nullptr) {
        unsigned long long qwCbp =
            static_cast<unsigned long long>(mPaletteVram->GetBlockAddr()) & kGsTbpMask;
        mTex0 = (mTex0 & ~(kGsTbpMask << kTex0CbpShift)) | (qwCbp << kTex0CbpShift);
        if ((mTex0 & (kGsTbpMask << kTex0CbpShift)) == 0) {
            qwCbp = static_cast<unsigned long long>(UploadPaletteClut()) & kGsTbpMask;
            mTex0 = (mTex0 & ~(kGsTbpMask << kTex0CbpShift)) | (qwCbp << kTex0CbpShift);
        }
    }

    const unsigned long long qwTbp0 =
        static_cast<unsigned long long>(mGsMips[0].mPage->GetBlockAddr()) & kGsTbpMask;
    mTex0 = (mTex0 & ~kGsTbpMask) | qwTbp0;
    if ((mTex0 & kGsTbpMask) == 0) {
        mTex0 = (mTex0 & ~kGsTbpMask) |
                (static_cast<unsigned long long>(UploadBitmapMipToGs(0)) & kGsTbpMask);
    }

    g_gfxDevice.SetGsReg(kGsRegTex0, mTex0, kGsRegAllBits);
    g_gfxDevice.SetGsReg(kGsRegTex1, mTex1, kGsRegAllBits);

    const int nMips = static_cast<int>(mLoadedBitmaps.size());
    for (int nMip = 1; nMip < nMips; ++nMip) {
        UploadMipAndBuildMipTbp(nMip, true);
    }
    if (nMips > kMipTbpLevel1) {
        g_gfxDevice.SetGsReg(kGsRegMipTbp1, mMipTbp1, kGsRegAllBits);
    }
    if (nMips > kMipTbpLevel4) {
        g_gfxDevice.SetGsReg(kGsRegMipTbp2, mMipTbp2, kGsRegAllBits);
    }
    return true;
}

// 0x00596d68
void PsTex::BindAsRenderTarget() {
    WaitForMipsLoaded();
    GsMip &mip = mGsMips[0];
    const ABitmap *pBitmap = mLoadedBitmaps[0];

    if (mip.mPage->mKind != kVramBlockKindRenderTarget) {
        mip.mPage->FreeSelf();
        mip.mPage = g_vramTable.AllocEntry();
        mip.mPage->SetupSurface(
            pBitmap->mWidth, pBitmap->mHeight, mBitsPerPixel, mGsPsm, kVramBlockKindRenderTarget);
    }
    int nBlockAddr = mip.mPage->GetBlockAddr();
    if (nBlockAddr == 0) {
        nBlockAddr = mip.mPage->UploadImage(nullptr, 0, 0, 0, 0);
    }

    // The binary builds FRAME_1 and XYOFFSET_1 in bit-field temporaries whose other fields it never
    // writes. The masks passed to SetGsReg() exclude those fields.
    const unsigned long long qwFrame =
        (static_cast<unsigned long long>(nBlockAddr >> kBlocksPerFramePageShift) & kFrameFbpMask) |
        (((mTex0 >> kTex0TbwShift) & kSixBitField) << kFrameFbwShift) |
        (((mTex0 >> kTex0PsmShift) & kSixBitField) << kFramePsmShift);
    const unsigned long long qwOffsetX =
        static_cast<unsigned long long>((kGsCoordinateCentre - (pBitmap->mWidth >> 1))
                                        << kGsSubpixelShift) &
        kGsSubpixelCoordMask;
    const unsigned long long qwOffsetY =
        static_cast<unsigned long long>((kGsCoordinateCentre - (pBitmap->mHeight >> 1))
                                        << kGsSubpixelShift) &
        kGsSubpixelCoordMask;
    g_gfxDevice.SetGsReg(kGsRegFrame1, qwFrame, kFrameFields);
    g_gfxDevice.SetGsReg(
        kGsRegXyOffset1, qwOffsetX | (qwOffsetY << kXyOffsetYShift), kXyOffsetFields);
    g_gfxDevice.SetGsReg(kGsRegZbuf1, kZbufZmsk, kZbufZmsk);
    g_gfxDevice.SetGsReg(kGsRegTest1, kTestZtstAlways, kTestZtstMask);
}

// 0x005982a0
int PsTex::UploadMipAndBuildMipTbp(int nMip, bool bSkipIfResident) {
    if (mLoadedBitmaps.empty() || mLoadedBitmaps[0] == nullptr) {
        return 0;
    }
    if (mGsMips.empty()) {
        return 0;
    }

    VramTableEntry *pPage = mGsMips[nMip].mPage;
    int nBlockAddr;
    if (nMip >= mFirstPackedMip) {
        nBlockAddr = mGsMips[mFirstPackedMip - 1].mPage->mMemAddr +
                     g_anPackedMipPageOffsets[nMip - mFirstPackedMip];
    } else {
        nBlockAddr = 0;
        if (bSkipIfResident) {
            nBlockAddr = pPage->GetBlockAddr();
        }
        if (nBlockAddr == 0) {
            nBlockAddr = UploadBitmapMipToGs(nMip);
            if (nMip == mFirstPackedMip - 1) {
                for (unsigned nPacked = mFirstPackedMip; nPacked < mLoadedBitmaps.size();
                     ++nPacked) {
                    UploadBitmapMipToSubImage(pPage,
                                              static_cast<int>(nPacked),
                                              g_anPackedMipPageOffsets[nPacked - mFirstPackedMip]);
                }
            }
        }
    }

    const unsigned long long qwTbp = static_cast<unsigned long long>(nBlockAddr) & kGsTbpMask;
    switch (nMip) {
    case kMipTbpLevelTex0:
        mTex0 = (mTex0 & ~kGsTbpMask) | qwTbp;
        break;
    case kMipTbpLevel1:
        mMipTbp1 = (mMipTbp1 & ~(kGsTbpMask << kMipTbpFirstShift)) | (qwTbp << kMipTbpFirstShift);
        break;
    case kMipTbpLevel2:
        mMipTbp1 = (mMipTbp1 & ~(kGsTbpMask << kMipTbpSecondShift)) | (qwTbp << kMipTbpSecondShift);
        break;
    case kMipTbpLevel3:
        mMipTbp1 = (mMipTbp1 & ~(kGsTbpMask << kMipTbpThirdShift)) | (qwTbp << kMipTbpThirdShift);
        break;
    case kMipTbpLevel4:
        mMipTbp2 = (mMipTbp2 & ~(kGsTbpMask << kMipTbpFirstShift)) | (qwTbp << kMipTbpFirstShift);
        break;
    case kMipTbpLevel5:
        mMipTbp2 = (mMipTbp2 & ~(kGsTbpMask << kMipTbpSecondShift)) | (qwTbp << kMipTbpSecondShift);
        break;
    case kMipTbpLevel6:
        mMipTbp2 = (mMipTbp2 & ~(kGsTbpMask << kMipTbpThirdShift)) | (qwTbp << kMipTbpThirdShift);
        break;
    default:
        break;
    }
    return nBlockAddr;
}

// 0x0059a7e8
void PsTex::FreeLoadedBitmaps() {
    FreeGsSurfaces();
    Tex::FreeLoadedBitmaps();
}

// 0x0059a818
void PsTex::SetGsPageInUse(bool bInUse) {
    WaitForMipsLoaded();
    mGsMips[0].mPage->SetPinned(bInUse);
}

// 0x0059a888
void PsTex::StaticInit() {
    g_pfnNewTex = NewPsTex;
    g_gfxDevice.SetGsReg(kGsRegTex1, kTex1DefaultFilter, kTex1FilterMask);
}

// 0x0059aa48
ACanvas *PsTex::LockMipBitmap(int nMip, [[maybe_unused]] int nUnknown, int nFlags) {
    WaitForMipsLoaded();
    if (static_cast<unsigned>(nMip) >= mLoadedBitmaps.size() || mLoadedBitmaps[nMip] == nullptr) {
        return nullptr;
    }

    if ((nFlags & kLockReadBack) != 0) {
        const int nBlockAddr = mGsMips[nMip].mPage->GetBlockAddr();
        if (nBlockAddr != 0) {
            g_vramTable.ReadBackBitmap(mLoadedBitmaps[nMip],
                                       static_cast<unsigned short>(nBlockAddr));
        }
    }
    mLockedMip = nMip;
    return mGsMips[nMip].mVramBitmap;
}

// 0x0059ab30
void PsTex::UnlockMipBitmap() {
    if (mLoadedBitmaps.empty()) {
        return;
    }
    // Yes, the binary indexes by the recorded level without range-checking it against the vector.
    if (mLoadedBitmaps[mLockedMip] == nullptr) {
        return;
    }
    mDirtyMips |= 1u << mLockedMip;
}

// 0x0059ab78
void PsTex::SetPalette(APalette *pPalette) {
    if (mLoadedBitmaps.empty() || mLoadedBitmaps[0] == nullptr) {
        return;
    }
    if (pPalette != nullptr) {
        mLoadedBitmaps[0]->mPalette->SetEntries(pPalette->mEntries, 0, pPalette->mEnd);
    }
    RebuildClut();
    mDirtyMips |= kDirtyClut;
}

// 0x0059abe8
void PsTex::AllocPaletteVram() {
    if (mPaletteVram != nullptr) {
        return;
    }
    if (mLoadedBitmaps.empty() || mLoadedBitmaps[0] == nullptr) {
        return;
    }
    mPaletteVram = g_vramTable.AllocPalEntry();
    if (mPaletteVram != nullptr) {
        mPaletteVram->ClearLockMask();
    } else {
        LogPrintf("Got NULL Palette in RestoreSurfaces\n");
    }
}

} // namespace Rnd
