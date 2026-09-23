#include "rnd/pstex.h"

#include <string.h>

#include "gfx/gfxdevice.h"
#include "gfx/vramtable.h"
#include "os/failsink.h"
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

// TEX0 fields RestoreSurfaces() assembles. TBP0 and CBP are left for the upload paths, and TFX for
// BindToGsSlot().
constexpr int kTex0TwShift = 26;
constexpr int kTex0ThShift = 30;
constexpr int kTex0TccShift = 34;
constexpr unsigned long long kFourBitField = 0xf;
constexpr unsigned long long kTex0TccBit = 1ULL << kTex0TccShift;
constexpr unsigned long long kTex0CpsmCsmCsaMask = 0x3ffULL << 51;
constexpr int kTex0CldShift = 61;
constexpr unsigned long long kTex0CldMask = 7ULL << kTex0CldShift;
constexpr unsigned long long kTex0CldLoadAlways = 1ULL << kTex0CldShift;

// TEX1 fields RestoreSurfaces() assembles, and the filters it chooses.
constexpr unsigned long long kTex1Lcm = 1;
constexpr int kTex1MxlShift = 2;
constexpr unsigned long long kTex1MxlMask = 7ULL << kTex1MxlShift;
constexpr unsigned long long kTex1MmagLinear = 1ULL << 5;
constexpr int kTex1MminShift = 6;
constexpr unsigned long long kTex1MminMask = 7ULL << kTex1MminShift;
constexpr unsigned long long kTex1MminLinear = 1;
constexpr unsigned long long kTex1MminLinearMipmapNearest = 4;
constexpr unsigned long long kTex1Mtba = 1ULL << 9;
constexpr unsigned long long kTex1LMask = 3ULL << 19;
constexpr int kTex1KShift = 32;
constexpr unsigned long long kTex1KMask = 0xfff;

// Where each MIPTBP register stores the buffer width of its three levels.
constexpr int kMipTbpFirstTbwShift = 14;
constexpr int kMipTbpSecondTbwShift = 34;
constexpr int kMipTbpThirdTbwShift = 54;

// Texels one TBW unit covers, and the rounding the four-bit and eight-bit formats need, whose
// buffer width has to be even.
constexpr int kTexelsPerTbwUnit = 64;
constexpr int kEvenTbwMask = 0xfffe;

// Mip levels smaller than this on their larger side share the page of the level before them.
constexpr short kPackedMipMaxSide = 32;

// Dimensions below this are rejected, as is any that is not a power of two.
constexpr int kMinMipSide = 8;

// The sentinel mFirstPackedMip stores while no level shares a page.
constexpr int kNoPackedMip = 9999;

// The eight-bit formats, which are the only ones whose small levels are packed.
constexpr int kPackedMipBitsPerPixel = 8;

// Bit of Rnd::Tex::mFlags under which the size check divides the width by three and the height
// by two. Rnd::Tex::DumpText() names the bit "CubeMap", and the three by two grid is the six faces.
constexpr int kTexSplitSizeCheck = 0x40;
constexpr int kSplitSizeColumns = 3;
constexpr int kSplitSizeRows = 2;

// The alpha byte of an 8888 colour, and how far it sits up the word.
constexpr int kAlphaShift = 24;
constexpr unsigned int kColorWithoutAlpha = 0x00ffffff;

// Map 0 to 255 source alpha onto the GS range of 0 to 128, rounding up. OnMipLoaded() open-codes
// the same expression in both of its loops.
inline unsigned int HalveAlpha(unsigned int nColor) {
    const unsigned int nAlpha = ((nColor >> kAlphaShift) + 1) >> 1;
    return (nColor & kColorWithoutAlpha) | (nAlpha << kAlphaShift);
}

// Report whether a dimension is a power of two of at least kMinMipSide.
inline bool IsValidMipSide(int nSide) {
    if (nSide < kMinMipSide) {
        return false;
    }
    while ((nSide & 1) == 0) {
        nSide >>= 1;
    }
    return nSide == 1;
}

// The exponent of the smallest power of two not below a dimension, which is what TEX0 TW and TH
// take.
inline int SideExponent(int nSide) {
    int nExponent = 0;
    for (int nPower = 1; nPower < nSide; nPower <<= 1) {
        ++nExponent;
    }
    return nExponent;
}

// The TBW of a bitmap, its width in 64-texel units, made even for the formats that need it.
inline int BufferWidthUnits(const ABitmap &bitmap) {
    int nUnits = (bitmap.mWidth + kTexelsPerTbwUnit - 1) / kTexelsPerTbwUnit;
    if (bitmap.mFormat == kABitmapFormatLinear4 || bitmap.mFormat == kABitmapFormatLinear8 ||
        bitmap.mFormat == kABitmapFormatRle8) {
        nUnits = (nUnits + 1) & kEvenTbwMask;
    }
    return nUnits;
}

// 0x0059a908
// Compare the palette of one level against that of level 0 entry by entry.
inline bool
CheckPalEqual(const APalette *pPalMip0, const APalette *pPalMip, const char *pszName, int nMip) {
    if (pPalMip0 == nullptr) {
        if (pPalMip != nullptr) {
            LogPrintf("CheckPalEqual(%s): Mipmap 0 has NULL palette!\n", pszName);
            return false;
        }
        return true;
    }
    if (pPalMip == nullptr) {
        LogPrintf("CheckPalEqual(%s): mipmap %d has NULL palette!\n", pszName, nMip);
        return false;
    }
    if (pPalMip0->mEnd != pPalMip->mEnd) {
        LogPrintf("CheckPalEqual(%s): Mipmap 0 pal is %d entries, mipmap %d is %d entries\n",
                  pszName,
                  pPalMip0->mEnd,
                  nMip,
                  pPalMip->mEnd);
        return false;
    }
    for (int nEntry = 0; nEntry < pPalMip0->mEnd; ++nEntry) {
        if (pPalMip0->mEntries[nEntry] != pPalMip->mEntries[nEntry]) {
            LogPrintf("CheckPalEqual(%s): mipmap 0 and %d unequal starting at index: %d\n",
                      pszName,
                      nMip,
                      nEntry);
            return false;
        }
    }
    return true;
}

// 0x0059a9e8
// Blank a level that failed validation. A run-length level is not modified.
inline void ClearBitmapPixels(ABitmap *pBitmap) {
    if (pBitmap->mFormat == kABitmapFormatRle8) {
        return;
    }
    if (pBitmap->mPalette != nullptr) {
        pBitmap->mPalette->SetEntries(&g_dwDefaultClutEntry, 0, 1);
    }
    memset(pBitmap->mPixels, 0, pBitmap->mByteCount);
}

} // namespace

// 0x0076f360
const unsigned int g_dwDefaultClutEntry = 0xff8080ff;

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

// 0x00597210
void PsTex::RestoreSurfaces() {
    if (mLoadedBitmaps.empty() || mLoadedBitmaps[0] == nullptr) {
        Tex::RestoreSurfaces();
        return;
    }
    if (mLoadedBitmaps[0]->mPalette != nullptr) {
        mDirtyMips |= kDirtyClut;
        RebuildClut();
        AllocPaletteVram();
    }

    mGsMips.resize(mLoadedBitmaps.size());

    const int nMips = static_cast<int>(mLoadedBitmaps.size());
    const ABitmap *pBase = mLoadedBitmaps[0];
    mBitsPerPixel = g_anBitsPerPixelTable[pBase->mFormat];
    mGsPsm = g_anGsPixelStorageModes[pBase->mFormat];
    mFirstPackedMip = kNoPackedMip;
    if (mBitsPerPixel == kPackedMipBitsPerPixel) {
        for (unsigned nMip = 1; nMip < mLoadedBitmaps.size(); ++nMip) {
            const ABitmap *pBitmap = mLoadedBitmaps[nMip];
            if (pBitmap == nullptr) {
                continue;
            }
            const short nLargerSide =
                pBitmap->mHeight < pBitmap->mWidth ? pBitmap->mWidth : pBitmap->mHeight;
            if (nLargerSide <= kPackedMipMaxSide) {
                mFirstPackedMip = static_cast<int>(nMip);
                break;
            }
        }
    }

    // The binary computes the unrounded TBW first and rounds it in place.
    mTex0 = (mTex0 & ~(kSixBitField << kTex0TbwShift)) |
            ((static_cast<unsigned long long>(BufferWidthUnits(*pBase)) & kSixBitField)
             << kTex0TbwShift);
    mTex0 =
        (mTex0 & ~(kSixBitField << kTex0PsmShift)) |
        ((static_cast<unsigned long long>(g_anGsPixelStorageModes[pBase->mFormat]) & kSixBitField)
         << kTex0PsmShift);
    mTex0 = (mTex0 & ~(kFourBitField << kTex0TwShift)) |
            ((static_cast<unsigned long long>(SideExponent(pBase->mWidth)) & kFourBitField)
             << kTex0TwShift);
    mTex0 = (mTex0 & ~(kFourBitField << kTex0ThShift)) |
            ((static_cast<unsigned long long>(SideExponent(pBase->mHeight)) & kFourBitField)
             << kTex0ThShift);

    unsigned long long qwTex1 = mTex1 & ~kTex1Lcm & ~kTex1MxlMask;
    qwTex1 |= (static_cast<unsigned long long>(nMips - 1) << kTex1MxlShift) & kTex1MxlMask;
    qwTex1 |= kTex1MmagLinear;
    qwTex1 &= ~kTex1MminMask;
    qwTex1 |= (nMips < 2 ? kTex1MminLinear : kTex1MminLinearMipmapNearest) << kTex1MminShift;
    qwTex1 &= ~kTex1Mtba & ~kTex1LMask;
    qwTex1 = (qwTex1 & ~(kTex1KMask << kTex1KShift)) |
             ((static_cast<unsigned long long>(mMipSelect) & kTex1KMask) << kTex1KShift);

    // A 24-bit format has no alpha to use, and CLD 1 reloads the CLUT on every TEX0 write.
    unsigned long long qwTex0 = mTex0 & ~kTex0TccBit;
    if (pBase->mFormat != kABitmapFormatLinear24) {
        qwTex0 |= kTex0TccBit;
    }
    qwTex0 &= ~(kTex0TfxMask << kTex0TfxShift) & ~kTex0CpsmCsmCsaMask & ~kTex0CldMask;
    mTex0 = qwTex0 | kTex0CldLoadAlways;
    mTex1 = qwTex1;

    const char *pszName = mName.mStr != nullptr ? mName.mStr : g_szEmptyString;
    for (int nMip = 0; nMip < nMips; ++nMip) {
        GsMip &mip = mGsMips[nMip];
        mip.mVramBitmap = nullptr;
        mip.mPage = nullptr;

        ABitmap *pBitmap = mLoadedBitmaps[nMip];
        if (pBitmap == nullptr) {
            LogPrintf("ERROR - RestoreSurfaces(%s), mipmap %d has no bm!\n", pszName, nMip);
            continue;
        }

        int nCheckWidth = pBitmap->mWidth;
        int nCheckHeight = pBitmap->mHeight;
        if ((mFlags & kTexSplitSizeCheck) != 0) {
            nCheckWidth /= kSplitSizeColumns;
            nCheckHeight /= kSplitSizeRows;
        }
        bool bValid = true;
        if (!IsValidMipSide(nCheckWidth) || !IsValidMipSide(nCheckHeight)) {
            const char *pszWhy = "not power-of-2";
            if (pBitmap->mWidth < kMinMipSide || pBitmap->mHeight < kMinMipSide) {
                pszWhy = "less than 8";
            }
            g_failSink.Report("bad: %s (mipmap %d) dimensions are %s (%d x %d)\n",
                              pszName,
                              nMip,
                              pszWhy,
                              pBitmap->mWidth,
                              pBitmap->mHeight);
            ClearBitmapPixels(pBitmap);
            bValid = false;
        }

        const char *pszPath = mBitmapPath.mStr != nullptr ? mBitmapPath.mStr : g_szEmptyString;
        if (nMip != 0 && bValid && mLoadedBitmaps[0]->mFormat != pBitmap->mFormat) {
            g_failSink.Report(
                "%s (file: %s, mipmap %d) doesn't have same bitmap format as original\n",
                pszName,
                pszPath,
                nMip);
            ClearBitmapPixels(pBitmap);
            bValid = false;
        }
        if (nMip != 0 && bValid &&
            !CheckPalEqual(mLoadedBitmaps[0]->mPalette, pBitmap->mPalette, pszName, nMip)) {
            g_failSink.Report("%s (file: %s, mipmap %d) doesn't have same pal as original\n",
                              pszName,
                              pszPath,
                              nMip);
            ClearBitmapPixels(pBitmap);
        }

        const unsigned long long qwTbw =
            static_cast<unsigned long long>(BufferWidthUnits(*pBitmap)) & kSixBitField;
        switch (nMip) {
        case kMipTbpLevel1:
            mMipTbp1 = (mMipTbp1 & ~(kSixBitField << kMipTbpFirstTbwShift)) |
                       (qwTbw << kMipTbpFirstTbwShift);
            break;
        case kMipTbpLevel2:
            mMipTbp1 = (mMipTbp1 & ~(kSixBitField << kMipTbpSecondTbwShift)) |
                       (qwTbw << kMipTbpSecondTbwShift);
            break;
        case kMipTbpLevel3:
            mMipTbp1 = (mMipTbp1 & ~(kSixBitField << kMipTbpThirdTbwShift)) |
                       (qwTbw << kMipTbpThirdTbwShift);
            break;
        case kMipTbpLevel4:
            mMipTbp2 = (mMipTbp2 & ~(kSixBitField << kMipTbpFirstTbwShift)) |
                       (qwTbw << kMipTbpFirstTbwShift);
            break;
        case kMipTbpLevel5:
            mMipTbp2 = (mMipTbp2 & ~(kSixBitField << kMipTbpSecondTbwShift)) |
                       (qwTbw << kMipTbpSecondTbwShift);
            break;
        case kMipTbpLevel6:
            mMipTbp2 = (mMipTbp2 & ~(kSixBitField << kMipTbpThirdTbwShift)) |
                       (qwTbw << kMipTbpThirdTbwShift);
            break;
        default:
            break;
        }

        mDirtyMips |= 1u << nMip;
        mip.mVramBitmap = ACanvas::CreateForBitmap(*pBitmap, false);
        mip.mVramBitmap->mBitmap.mPixels = pBitmap->mPixels;
        mip.mVramBitmap->mBitmap.mPalette = pBitmap->mPalette;
        mip.mPage = g_vramTable.AllocEntry();
        mip.mPage->SetupSurface(
            pBitmap->mWidth, pBitmap->mHeight, mBitsPerPixel, mGsPsm, kVramBlockKindTexture);
    }
    Tex::RestoreSurfaces();
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

// 0x0059a770
Tex *NewPsTex(const HxStr &name) {
    // The binary bills the 0x4b0-byte allocation to the tag "Rnd::Tex".
    return new PsTex(name);
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
void PsTex::SetPalette(APalette *pPalette, [[maybe_unused]] int nUnknown) {
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
