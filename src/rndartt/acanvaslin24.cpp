#include "rndartt/acanvaslin24.h"

#include <string.h>

#include "rndartt/apalette.h"
#include "rndartt/apoint.h"
#include "rndartt/arowspan.h"
#include "rndartt/astretchblit.h"

namespace {

constexpr int kBytesPerPixel = 3;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;

// Three bytes are one pixel. The binary forms the column offset as the index doubled plus itself
// rather than with a multiply.
inline unsigned char *PixelAt(void *pPixels, int nBytesPerRow, int nX, int nY) {
    void *pPixel = static_cast<unsigned char *>(pPixels) + (nY * nBytesPerRow);
    return static_cast<unsigned char *>(pPixel) + (nX * kBytesPerPixel);
}

inline const unsigned char *SourceByteAt(const ABitmap &source, int nX, int nY) {
    return static_cast<const unsigned char *>(source.mPixels) + (nY * source.mBytesPerRow) + nX;
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
    return ACanvas::palDefault;
}

// A palette entry keeps red in its low byte, so the three channels are its three low bytes.
inline void StoreEntry(unsigned char *pPixel, unsigned int nEntry) {
    pPixel[1] = static_cast<unsigned char>(nEntry >> kGreenShift);
    pPixel[2] = static_cast<unsigned char>(nEntry >> kBlueShift);
    pPixel[0] = static_cast<unsigned char>(nEntry);
}

// The binary assembles the three channel bytes and a zero into a stack word and compares that
// word, which is the little endian value formed here.
inline unsigned int KeyFromChannels(const unsigned char *pRGB) {
    return static_cast<unsigned int>(pRGB[0] | (pRGB[1] << kGreenShift) | (pRGB[2] << kBlueShift));
}

} // namespace

// NTSC-U/C: 0x00618470, PAL: 0x00659000
ACanvasLin24::ACanvasLin24(const ABitmap &bitmap) : ACanvas24(bitmap) {
    mColorNative = 0;
}

// NTSC-U/C: 0x006184a8, PAL: 0x00659038
// Empty in the binary, and the slot exists only because ACanvas24 declares it pure.
void ACanvasLin24::SetAlphaValues(unsigned int nColorKey) {
    (void)nColorKey;
}

// NTSC-U/C: 0x006184b0, PAL: 0x00659040
void ACanvasLin24::DrawPixelU(int nX, int nY) {
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    pPixel[0] = mColorChannels[0];
    pPixel[1] = mColorChannels[1];
    pPixel[2] = mColorChannels[2];
}

// NTSC-U/C: 0x006184e8, PAL: 0x00659078
void ACanvasLin24::DrawPixel24U(int nX, int nY, const unsigned char *pRGB) {
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    pPixel[0] = pRGB[0];
    pPixel[1] = pRGB[1];
    pPixel[2] = pRGB[2];
}

// NTSC-U/C: 0x00618528, PAL: 0x006590b8
void ACanvasLin24::GetPixel24U(int nX, int nY, unsigned char *pRGB) {
    const unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    pRGB[0] = pPixel[0];
    pRGB[1] = pPixel[1];
    pRGB[2] = pPixel[2];
}

// NTSC-U/C: 0x00618568, PAL: 0x006590f8
// The three colour bytes are re-read from the object on every iteration rather than
// hoisted, and the destination advances one byte at a time rather than three at once.
void ACanvasLin24::DrawHorzLineU(int nY, int nLeft, int nRight) {
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nLeft, nY);
    for (int nCount = nRight - nLeft; nCount != 0; --nCount) {
        *pPixel = mColorChannels[0];
        ++pPixel;
        *pPixel = mColorChannels[1];
        ++pPixel;
        *pPixel = mColorChannels[2];
        ++pPixel;
    }
}

// NTSC-U/C: 0x006185d0, PAL: 0x00659160
// The row pitch is re-read from the bitmap on every iteration.
void ACanvasLin24::DrawVertLineU(int nX, int nTop, int nBottom) {
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nTop);
    for (int nCount = nBottom - nTop; nCount != 0; --nCount) {
        pPixel[0] = mColorChannels[0];
        pPixel[1] = mColorChannels[1];
        pPixel[2] = mColorChannels[2];
        pPixel += mBitmap.mBytesPerRow;
    }
}

// NTSC-U/C: 0x00618630, PAL: 0x006591c0
void ACanvasLin24::DrawRectU(ARect rect) {
    const short nColumns = static_cast<short>(rect.mRight - rect.mLeft);
    const int nRowAdvance = mBitmap.mBytesPerRow - (nColumns * kBytesPerPixel);
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, rect.mLeft, rect.mTop);
    for (short nRows = static_cast<short>(rect.mBottom - rect.mTop); nRows > 0; --nRows) {
        for (int nCount = nColumns; nCount != 0; --nCount) {
            *pPixel = mColorChannels[0];
            ++pPixel;
            *pPixel = mColorChannels[1];
            ++pPixel;
            *pPixel = mColorChannels[2];
            ++pPixel;
        }
        pPixel += nRowAdvance;
    }
}

// NTSC-U/C: 0x00618a28, PAL: 0x006595b8
void ACanvasLin24::DrawTmapRow8U(int nY,
                                 int nLeft,
                                 int nRight,
                                 const ABitmap *pSource,
                                 APoint *pSourcePosition,
                                 const APoint *pSourceStep) {
    const APalette *pPalette = ResolvePalette(*pSource, mBitmap);
    if (pPalette == nullptr) {
        return;
    }
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nLeft, nY);
    for (int nRemaining = nRight - nLeft; nRemaining > 0; --nRemaining) {
        const unsigned char nIndex = *SourceByteAt(*pSource,
                                                   pSourcePosition->mX >> kACanvasFractionBits,
                                                   pSourcePosition->mY >> kACanvasFractionBits);
        StoreEntry(pDest, pPalette->mEntries[nIndex]);
        pDest += kBytesPerPixel;
        pSourcePosition->mX += pSourceStep->mX;
        pSourcePosition->mY += pSourceStep->mY;
    }
}

// NTSC-U/C: 0x006186f8, PAL: 0x00659288
// The key is compared against the low byte of the transparent colour, and the flag is
// re-read for every pixel.
void ACanvasLin24::DrawBitmapLin8U(const ABitmap &source, int nX, int nY) {
    const APalette *pPalette = ResolvePalette(source, mBitmap);
    if (pPalette == nullptr) {
        return;
    }
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    const unsigned char *pIndex = static_cast<const unsigned char *>(source.mPixels);
    for (int nRows = source.mHeight; nRows > 0; --nRows) {
        for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
            if (source.mHasTransparentColor == 0 ||
                *pIndex != static_cast<unsigned char>(source.mTransparentColor)) {
                StoreEntry(pDest, pPalette->mEntries[*pIndex]);
            }
            ++pIndex;
            pDest += kBytesPerPixel;
        }
        pIndex += source.mBytesPerRow - source.mWidth;
        pDest += mBitmap.mBytesPerRow - (kBytesPerPixel * source.mWidth);
    }
}

// NTSC-U/C: 0x00618270, PAL: 0x00658e00
// Unlike the eight and 1555 layouts there is no whole-rectangle copy tier. A keyed row
// compares the pixel's three bytes, widened with a zero, against the whole transparent colour.
void ACanvasLin24::DrawBitmapLin24U(const ABitmap &source, int nX, int nY) {
    const unsigned char *pSourceByte = static_cast<const unsigned char *>(source.mPixels);
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    for (int nRows = source.mHeight; nRows > 0; --nRows) {
        if (source.mHasTransparentColor != 0) {
            const int nRowBytes = kBytesPerPixel * source.mWidth;
            for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
                if (source.mTransparentColor != KeyFromChannels(pSourceByte)) {
                    pDest[0] = pSourceByte[0];
                    pDest[1] = pSourceByte[1];
                    pDest[2] = pSourceByte[2];
                }
                pSourceByte += kBytesPerPixel;
                pDest += kBytesPerPixel;
            }
            pSourceByte += source.mBytesPerRow - nRowBytes;
            pDest += mBitmap.mBytesPerRow - nRowBytes;
        } else {
            memcpy(pDest, pSourceByte, static_cast<unsigned int>(kBytesPerPixel * source.mWidth));
            pSourceByte += source.mBytesPerRow;
            pDest += mBitmap.mBytesPerRow;
        }
    }
}

// NTSC-U/C: 0x00618800, PAL: 0x00659390
// The key is compared against the low byte of the transparent colour.
void ACanvasLin24::DrawClutBitmapRowLin8U(const ARowInfo &span, const unsigned char *pRemap) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    const unsigned char *pIndex = span.mSource;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        if (!span.mHasTransparentColor ||
            *pIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            StoreEntry(pDest, span.mPalette->mEntries[pRemap[*pIndex]]);
        }
        ++pIndex;
        pDest += kBytesPerPixel;
    }
}

// NTSC-U/C: 0x006188b0, PAL: 0x00659440
void ACanvasLin24::DrawScaledBitmapRowLin8U(const AScaledRowInfo &span) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            StoreEntry(pDest, span.mPalette->mEntries[nIndex]);
        }
        pDest += kBytesPerPixel;
        nPosition += span.mSourceStep;
    }
}

// NTSC-U/C: 0x00618968, PAL: 0x006594f8
void ACanvasLin24::DrawScaledClutBitmapRowLin8U(const AScaledRowInfo &span,
                                                const unsigned char *pRemap) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            StoreEntry(pDest, span.mPalette->mEntries[pRemap[nIndex]]);
        }
        pDest += kBytesPerPixel;
        nPosition += span.mSourceStep;
    }
}
