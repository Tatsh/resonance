#include "rndartt/acanvaslin15.h"

#include <string.h>

#include "rndartt/apalette.h"
#include "rndartt/apoint.h"
#include "rndartt/arowspan.h"
#include "rndartt/astretchspan.h"

namespace {

constexpr unsigned int kColor15Mask = 0x7fff;
constexpr unsigned int kAlpha15Bit = 0x8000;
constexpr int kBytesPerPixel = 2;

// The top five bits of the red and green bytes of an 8888 colour, the 1555 blue field, and the
// shift that moves each 8888 field into its 1555 position.
constexpr unsigned int kRedHigh5Mask = 0x0000f8;
constexpr unsigned int kGreenHigh5Mask = 0x00f800;
constexpr unsigned int kBlue15Mask = 0x7c00;
constexpr int kRedPackShift = 3;
constexpr int kGreenPackShift = 6;
constexpr int kBluePackShift = 9;
constexpr int kAlphaPackShift = 16;

// Two bytes are one pixel. The row offset is a byte count and the column is a pixel index, which
// is how the binary computes it: the row is added as bytes and the column shifted left by one.
inline unsigned short *RowAt(void *pPixels, int nBytesPerRow, int nY) {
    void *pRow = static_cast<unsigned char *>(pPixels) + (nY * nBytesPerRow);
    return static_cast<unsigned short *>(pRow);
}

inline unsigned short *PixelAt(void *pPixels, int nBytesPerRow, int nX, int nY) {
    return RowAt(pPixels, nBytesPerRow, nY) + nX;
}

inline const unsigned char *SourceByteAt(const ABitmap &source, int nX, int nY) {
    return static_cast<const unsigned char *>(source.mPixels) + (nY * source.mBytesPerRow) + nX;
}

// Blit15NoClip() walks both rectangles as bytes, because its row advance is not a whole number of
// pixels.
inline const unsigned short *HalfwordAt(const unsigned char *pByte) {
    return static_cast<const unsigned short *>(static_cast<const void *>(pByte));
}

inline unsigned short *HalfwordAt(unsigned char *pByte) {
    return static_cast<unsigned short *>(static_cast<void *>(pByte));
}

// The source, then the canvas, then the global default. Every palette reading override of this
// class opens with the same three tests.
inline const APalette *ResolvePalette(const ABitmap &source, const ABitmap &canvas) {
    if (source.mPalette != nullptr) {
        return source.mPalette;
    }
    if (canvas.mPalette != nullptr) {
        return canvas.mPalette;
    }
    return g_pDefaultPalette;
}

} // namespace

// 0x00618f38
unsigned short APackRgb1555From8888(unsigned int nColor) {
    return static_cast<unsigned short>(((nColor & kRedHigh5Mask) >> kRedPackShift) |
                                       ((nColor & kGreenHigh5Mask) >> kGreenPackShift) |
                                       ((nColor >> kBluePackShift) & kBlue15Mask) |
                                       ((nColor >> kAlphaPackShift) & kAlpha15Bit));
}

// 0x00619008
ACanvasLin15::ACanvasLin15(const ABitmap &bitmap) : ACanvas15(bitmap) {
    mColor = 0;
}

// 0x00618f68
ACanvasLin15::~ACanvasLin15() {
}

// 0x00619040
// The width, the height, and the row pitch are all re-read inside the loops rather
// than hoisted, which is what the binary does.
void ACanvasLin15::BuildAlphaFromColorKey(unsigned int nColorKey) {
    const unsigned int nKey = nColorKey & 0xffff;
    for (int nY = 0; nY < mBitmap.mHeight; ++nY) {
        unsigned short *pPixel = RowAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nY);
        for (int nX = 0; nX < mBitmap.mWidth; ++nX) {
            const unsigned int nColor = *pPixel;
            if ((nColor & kColor15Mask) != nKey) {
                *pPixel = static_cast<unsigned short>(nColor | kAlpha15Bit);
            } else {
                *pPixel = static_cast<unsigned short>(nColor & kColor15Mask);
            }
            ++pPixel;
        }
    }
}

// 0x006190c0
void ACanvasLin15::PutPixelNoClip(int nX, int nY) {
    *PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY) = mColor;
}

// 0x006190e8
void ACanvasLin15::PutPixel15NoClip(int nX, int nY, unsigned short nColor) {
    *PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY) = nColor;
}

// 0x00619108
unsigned short ACanvasLin15::GetPixel15NoClip(int nX, int nY) {
    return *PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
}

// 0x00619128
// A halfword store per pixel rather than a memset, and the colour is re-read on every
// iteration.
void ACanvasLin15::FillRowNoClip(int nY, int nLeft, int nRight) {
    unsigned short *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nLeft, nY);
    for (int nCount = nRight - nLeft; nCount != 0; --nCount) {
        *pPixel = mColor;
        ++pPixel;
    }
}

// 0x00619170
// The colour and the row pitch are both re-read on every iteration, which recomputing
// the address per row reproduces.
void ACanvasLin15::FillColumnNoClip(int nX, int nTop, int nBottom) {
    for (int nY = nTop; nY < nBottom; ++nY) {
        *PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY) = mColor;
    }
}

// 0x006191c0
void ACanvasLin15::FillRectNoClip(ARect rect) {
    const int nColumns = rect.mRight - rect.mLeft;
    unsigned short *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, rect.mLeft, rect.mTop);
    for (int nRows = rect.mBottom - rect.mTop; nRows > 0; --nRows) {
        for (int nCount = nColumns; nCount != 0; --nCount) {
            *pPixel = mColor;
            ++pPixel;
        }
        // The advance is the pitch converted to pixels, less the span just written, converted back
        // to bytes. That halving and doubling is what the binary computes, and it differs from the
        // pitch less twice the span whenever the pitch is odd.
        pPixel += (mBitmap.mBytesPerRow / kBytesPerPixel) - nColumns;
    }
}

// 0x00619518
void ACanvasLin15::TextureRowIndexed(int nY,
                                     int nLeft,
                                     int nRight,
                                     const ABitmap *pSource,
                                     APoint *pSourcePosition,
                                     const APoint *pSourceStep) {
    const APalette *pPalette = ResolvePalette(*pSource, mBitmap);
    if (pPalette == nullptr) {
        return;
    }
    unsigned short *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nLeft, nY);
    for (int nRemaining = nRight - nLeft; nRemaining > 0; --nRemaining) {
        const unsigned char nIndex = *SourceByteAt(*pSource,
                                                   pSourcePosition->mX >> kACanvasFractionBits,
                                                   pSourcePosition->mY >> kACanvasFractionBits);
        *pDest = APackRgb1555From8888(pPalette->mEntries[nIndex]);
        ++pDest;
        pSourcePosition->mX += pSourceStep->mX;
        pSourcePosition->mY += pSourceStep->mY;
    }
}

// 0x00618b00
// Each row is unpacked into g_abCanvasRowScratch first, and the key is compared against
// the low byte of the transparent colour.
void ACanvasLin15::Blit4NoClip(const ABitmap &source, int nX, int nY) {
    const APalette *pPalette = ResolvePalette(source, mBitmap);
    if (pPalette == nullptr) {
        return;
    }
    unsigned short *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    for (int nRows = source.mHeight; nRows > 0; --nRows) {
        UnpackNibbleRow(pSourceRow, g_abCanvasRowScratch, source.mWidth, source.mOddNibbleStart);
        const unsigned char *pIndex = g_abCanvasRowScratch;
        for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
            if (source.mHasTransparentColor == 0 ||
                *pIndex != static_cast<unsigned char>(source.mTransparentColor)) {
                *pDest = APackRgb1555From8888(pPalette->mEntries[*pIndex]);
            }
            ++pIndex;
            ++pDest;
        }
        pSourceRow += source.mBytesPerRow;
        pDest += (mBitmap.mBytesPerRow / kBytesPerPixel) - source.mWidth;
    }
}

// 0x00618c78
// The transparency flag is tested once per row, choosing between a keyed and an
// opaque walk.
void ACanvasLin15::Blit8NoClip(const ABitmap &source, int nX, int nY) {
    const APalette *pPalette = ResolvePalette(source, mBitmap);
    if (pPalette == nullptr) {
        return;
    }
    unsigned short *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    const unsigned char *pIndex = static_cast<const unsigned char *>(source.mPixels);
    for (int nRows = source.mHeight; nRows > 0; --nRows) {
        if (source.mHasTransparentColor != 0) {
            for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
                if (*pIndex != static_cast<unsigned char>(source.mTransparentColor)) {
                    *pDest = APackRgb1555From8888(pPalette->mEntries[*pIndex]);
                }
                ++pIndex;
                ++pDest;
            }
        } else {
            for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
                *pDest = APackRgb1555From8888(pPalette->mEntries[*pIndex]);
                ++pIndex;
                ++pDest;
            }
        }
        pIndex += source.mBytesPerRow - source.mWidth;
        pDest += (mBitmap.mBytesPerRow / kBytesPerPixel) - source.mWidth;
    }
}

// 0x00618e00
// Three tiers, as in ACanvasLin8::Blit8NoClip().
void ACanvasLin15::Blit15NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pSourceByte = static_cast<const unsigned char *>(source.mPixels);
    unsigned char *pDestByte = static_cast<unsigned char *>(
        static_cast<void *>(PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY)));
    if (source.mHasTransparentColor == 0 && source.mBytesPerRow == mBitmap.mBytesPerRow) {
        memcpy(pDestByte, pSourceByte, static_cast<unsigned int>(source.mByteCount));
        return;
    }
    for (int nRows = source.mHeight; nRows > 0; --nRows) {
        if (source.mHasTransparentColor != 0) {
            for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
                if (*HalfwordAt(pSourceByte) !=
                    static_cast<unsigned short>(source.mTransparentColor)) {
                    *HalfwordAt(pDestByte) = *HalfwordAt(pSourceByte);
                }
                pSourceByte += kBytesPerPixel;
                pDestByte += kBytesPerPixel;
            }
            // Yes, the binary advances both rectangles by the pitch less the width in PIXELS after
            // a keyed row, which starts the next row mWidth bytes past where it belongs.
            pSourceByte += source.mBytesPerRow - source.mWidth;
            pDestByte += mBitmap.mBytesPerRow - source.mWidth;
        } else {
            memcpy(
                pDestByte, pSourceByte, static_cast<unsigned int>(kBytesPerPixel * source.mWidth));
            pSourceByte += source.mBytesPerRow;
            pDestByte += mBitmap.mBytesPerRow;
        }
    }
}

// 0x00619280
// The key is compared against the low byte of the transparent colour.
void ACanvasLin15::RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned short *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    const unsigned char *pIndex = span.mSource;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        if (!span.mHasTransparentColor ||
            *pIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            *pDest = APackRgb1555From8888(span.mPalette->mEntries[pRemap[*pIndex]]);
        }
        ++pIndex;
        ++pDest;
    }
}

// 0x00619360
// The key comparison here is against the whole transparent colour word, where
// StretchRowRemap() compares its low byte.
void ACanvasLin15::StretchRowIndexed(const AStretchSpan &span) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned short *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 || nIndex != span.mTransparentColor) {
            *pDest = APackRgb1555From8888(span.mPalette->mEntries[nIndex]);
        }
        ++pDest;
        nPosition += span.mSourceStep;
    }
}

// 0x00619430
void ACanvasLin15::StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned short *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            *pDest = APackRgb1555From8888(span.mPalette->mEntries[pRemap[nIndex]]);
        }
        ++pDest;
        nPosition += span.mSourceStep;
    }
}
