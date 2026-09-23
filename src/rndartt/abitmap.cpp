#include "rndartt/abitmap.h"

#include <string.h>

#include "os/mem.h"
#include "rndartt/acanvas.h"
#include "rndartt/apalette.h"

namespace {

// The only source path the shipped image retains for this file.
// 0x0082b1c0
const char *const kAllocTag = "abitmap.h";
constexpr int kAllocLine = 0x47;

constexpr int kBitsPerPixel4 = 4;
constexpr int kBitsPerPixel8 = 8;
constexpr int kBitsPerPixel16 = 16;
constexpr int kBitsPerPixel24 = 24;
constexpr int kBitsPerPixel32 = 32;
constexpr int kBytesPerPixel24 = 3;
constexpr int kBytesPerPixel32 = 4;

constexpr unsigned int kRed15Mask = 0x001f;
constexpr unsigned int kGreen15Mask = 0x03e0;
constexpr unsigned int kAlpha15Bit = 0x8000;
constexpr int kBlue15Shift = 10;
constexpr unsigned int kWhite15 = 0x7fff;
constexpr unsigned int kColorChannelsMask = 0xffffff;
constexpr unsigned int kLowByteMask = 0xff;
constexpr int kAlphaShift = 24;

// The value Copy() returns when the allocation fails.
constexpr int kCopyFailed = -1;

// The packed row stride of a format, as the constructor and Copy() both derive it.
inline short PackedBytesPerRow(int nFormat, int nWidth) {
    if (nFormat == kABitmapFormatLinear4) {
        return static_cast<short>((nWidth + 2) / 2);
    }
    return static_cast<short>(nWidth * g_abBitmapBytesPerPixel[nFormat]);
}

} // namespace

// 0x00725cc0
const unsigned char g_abBitmapBytesPerPixel[kABitmapFormatCount] = {0, 1, 2, 3, 4, 1};

// 0x00725cc8
const unsigned char g_abBitmapBitsPerPixel[kABitmapFormatCount] = {4, 8, 16, 24, 32, 8};

// 0x005593a0
ABitmap::ABitmap(void *pPixels,
                 int nFormat,
                 bool bHasTransparentColor,
                 int nWidth,
                 int nHeight,
                 int nBytesPerRow) {
    mHeight = static_cast<short>(nHeight);
    mFormat = nFormat;
    mHasTransparentColor = bHasTransparentColor;
    mOddNibbleStart = 0;
    mWidth = static_cast<short>(nWidth);
    if (nBytesPerRow != 0) {
        mBytesPerRow = static_cast<short>(nBytesPerRow);
    } else {
        mBytesPerRow = PackedBytesPerRow(mFormat, mWidth);
    }
    mTransparentColor = 0;
    mByteCount = mBytesPerRow * mHeight;
    mPixels = pPixels;
    if (pPixels == nullptr) {
        mPixels = MemAllocTagged(mByteCount, kAllocTag, kAllocLine);
        mOwnsPixels = 1;
    } else {
        mOwnsPixels = 0;
    }
    mPalette = nullptr;
}

// 0x00559310
int ABitmap::FormatForBitsPerPixel(int nBitsPerPixel) {
    switch (nBitsPerPixel) {
    case kBitsPerPixel4:
        return kABitmapFormatLinear4;
    case kBitsPerPixel8:
        return kABitmapFormatLinear8;
    case kBitsPerPixel16:
        return kABitmapFormatLinear15;
    case kBitsPerPixel24:
        return kABitmapFormatLinear24;
    case kBitsPerPixel32:
        return kABitmapFormatLinear32;
    default:
        return kABitmapFormatLinear15;
    }
}

// 0x00559368
int ABitmap::ComputeByteCount(int nFormat, int nWidth, int nHeight) {
    int nBytesPerRow;
    if (nFormat == kABitmapFormatLinear4) {
        nBytesPerRow = (nWidth + 2) / 2;
    } else {
        nBytesPerRow = nWidth * g_abBitmapBytesPerPixel[nFormat];
    }
    return nBytesPerRow * nHeight;
}

// 0x00559568
void ABitmap::SwapRedBlue15(unsigned short *pPixels, int nCount) {
    for (int nRemaining = nCount; nRemaining > 0; --nRemaining) {
        const unsigned int nColor = *pPixels;
        *pPixels++ = static_cast<unsigned short>(((nColor & kRed15Mask) << kBlue15Shift) |
                                                 (nColor & kAlpha15Bit) | (nColor & kGreen15Mask) |
                                                 ((nColor >> kBlue15Shift) & kRed15Mask));
    }
}

// 0x005595c8
void ABitmap::SwapRedBlue24(unsigned char *pPixels, int nCount) {
    for (int nRemaining = nCount; nRemaining > 0; --nRemaining) {
        const unsigned char nFirst = pPixels[0];
        pPixels[0] = pPixels[2];
        pPixels[2] = nFirst;
        pPixels += kBytesPerPixel24;
    }
}

// 0x00559600
void ABitmap::SwapRedBlue32(unsigned char *pPixels, int nCount) {
    for (int nRemaining = nCount; nRemaining > 0; --nRemaining) {
        const unsigned char nFirst = pPixels[0];
        pPixels[0] = pPixels[2];
        pPixels[2] = nFirst;
        pPixels += kBytesPerPixel32;
    }
}

// 0x00725cd0
int g_nSkipColorSwap;

// 0x00558f28
int ABitmap::Copy(const ABitmap &source) {
    mFormat = source.mFormat;
    mHasTransparentColor = source.mHasTransparentColor;
    mOddNibbleStart = source.mOddNibbleStart;
    mWidth = source.mWidth;
    mHeight = source.mHeight;
    mBytesPerRow = source.mBytesPerRow;
    mTransparentColor = source.mTransparentColor;
    mByteCount = source.mByteCount;
    mPalette = source.mPalette;
    if (mFormat != kABitmapFormatRle8 && mBytesPerRow != PackedBytesPerRow(mFormat, mWidth)) {
        mBytesPerRow = PackedBytesPerRow(mFormat, source.mWidth);
        mByteCount = mBytesPerRow * mHeight;
    }

    mPixels = MemAllocTagged(mByteCount, kAllocTag, kAllocLine);
    if (mPixels == nullptr) {
        return kCopyFailed;
    }
    mOwnsPixels = 1;

    if (mBytesPerRow == source.mBytesPerRow) {
        memcpy(mPixels, source.mPixels, mByteCount);
        return 0;
    }
    unsigned char *pDestRow = static_cast<unsigned char *>(mPixels);
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    for (int nRow = 0; nRow < mHeight; ++nRow) {
        memcpy(pDestRow, pSourceRow, mBytesPerRow);
        pDestRow += mBytesPerRow;
        pSourceRow += source.mBytesPerRow;
    }
    return 0;
}

// 0x00559140
void ABitmap::SwapRedBlue() {
    unsigned char *pRow = static_cast<unsigned char *>(mPixels);
    switch (mFormat) {
    case kABitmapFormatLinear4:
    case kABitmapFormatLinear8:
    case kABitmapFormatRle8:
        if (mPalette != nullptr) {
            SwapRedBlue32(reinterpret_cast<unsigned char *>(mPalette->mEntries), mPalette->mEnd);
        }
        break;
    case kABitmapFormatLinear15:
        for (int nRow = 0; nRow < mHeight; ++nRow) {
            SwapRedBlue15(reinterpret_cast<unsigned short *>(pRow), mWidth);
            pRow += mBytesPerRow;
        }
        break;
    case kABitmapFormatLinear24:
        for (int nRow = 0; nRow < mHeight; ++nRow) {
            SwapRedBlue24(pRow, mWidth);
            pRow += mBytesPerRow;
        }
        break;
    case kABitmapFormatLinear32:
        for (int nRow = 0; nRow < mHeight; ++nRow) {
            SwapRedBlue32(pRow, mWidth);
            pRow += mBytesPerRow;
        }
        break;
    default:
        break;
    }
}

// 0x004e5b78
void ABitmap::ApplyColorKey(int nFlags) {
    if (nFlags != 0) {
        if (mPalette != nullptr &&
            (mFormat == kABitmapFormatLinear4 || mFormat == kABitmapFormatLinear8 ||
             mFormat == kABitmapFormatRle8)) {
            for (int i = 0; i < mPalette->mEnd; ++i) {
                const unsigned int nColor = mPalette->mEntries[i] & kColorChannelsMask;
                if ((nFlags & kABitmapColorKeyWhite) != 0 && nColor == kColorChannelsMask) {
                    mTransparentColor = i;
                    mHasTransparentColor = 1;
                    break;
                }
                if ((nFlags & kABitmapColorKeyBlack) != 0 && nColor == 0) {
                    mHasTransparentColor = 1;
                    mTransparentColor = i;
                    break;
                }
            }
        } else if (mFormat == kABitmapFormatLinear15) {
            if ((nFlags & kABitmapColorKeyWhite) != 0) {
                mHasTransparentColor = 1;
                mTransparentColor = kWhite15;
            } else if ((nFlags & kABitmapColorKeyBlack) != 0) {
                mTransparentColor = 0;
                mHasTransparentColor = 1;
            }
        } else if (mFormat == kABitmapFormatLinear32) {
            if ((nFlags & kABitmapColorKeyWhite) != 0) {
                mHasTransparentColor = 1;
                mTransparentColor = kColorChannelsMask;
            } else if ((nFlags & kABitmapColorKeyBlack) != 0) {
                mTransparentColor = 0;
                mHasTransparentColor = 1;
            }
        }
    }
    if (mHasTransparentColor != 0) {
        ACanvas *pCanvas = ACanvas::CreateForBitmap(*this, false);
        pCanvas->BuildAlphaFromColorKey(mTransparentColor);
        delete pCanvas;
    }
}

// 0x004e7d48
void ABitmap::SetPaletteAlphaFromLowByte(int bWhiten) {
    if (mPalette == nullptr) {
        return;
    }
    for (int i = 0; i < mPalette->mEnd; ++i) {
        const unsigned int nEntry = mPalette->mEntries[i];
        const unsigned int nColor = bWhiten != 0 ? kColorChannelsMask : nEntry & kColorChannelsMask;
        mPalette->mEntries[i] = ((nEntry & kLowByteMask) << kAlphaShift) | nColor;
    }
}

// 0x005eb290
void ABitmap::SetPaletteEntries(const unsigned int *pEntries, int nFirst, int nCount) {
    if (mPalette == nullptr) {
        mPalette = new APalette;
    }
    if (mPalette != nullptr) {
        mPalette->SetEntries(pEntries, nFirst, nCount);
    }
}
