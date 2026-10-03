#include "rndartt/acanvaslin32.h"

#include <string.h>

#include "rndartt/apalette.h"
#include "rndartt/apoint.h"
#include "rndartt/arowspan.h"
#include "rndartt/astretchspan.h"

namespace {

constexpr int kBytesPerPixel = 4;

inline unsigned char *ByteAt(const ABitmap &bitmap, int nX, int nY) {
    return static_cast<unsigned char *>(bitmap.mPixels) + nY * bitmap.mBytesPerRow +
           kBytesPerPixel * nX;
}

inline unsigned int *PixelAt(const ABitmap &bitmap, int nX, int nY) {
    return reinterpret_cast<unsigned int *>(ByteAt(bitmap, nX, nY));
}

inline const unsigned int *ConstPixelAt(const ABitmap &bitmap, int nX, int nY) {
    return reinterpret_cast<const unsigned int *>(ByteAt(bitmap, nX, nY));
}

// Resolve the palette a copy should read entries from: the source, then the canvas, then the
// global default. Every palette reading override of this class opens with the same three tests.
inline APalette *ResolvePalette(const ABitmap &source, const ABitmap &canvas) {
    if (source.mPalette != nullptr) {
        return source.mPalette;
    }
    if (canvas.mPalette != nullptr) {
        return canvas.mPalette;
    }
    return g_pDefaultPalette;
}

} // namespace

// NTSC-U/C: 0x00614240, PAL: 0x00654dd0
ACanvasLin32::ACanvasLin32(const ABitmap &bitmap) : ACanvas32(bitmap) {
}

// NTSC-U/C: 0x00614278, PAL: 0x00654e08
void ACanvasLin32::BuildAlphaFromColorKey(unsigned int nColorKey) {
    unsigned char *pRow = static_cast<unsigned char *>(mBitmap.mPixels);
    nColorKey &= kACanvas32ChannelsMask;
    for (int y = 0; y < mBitmap.mHeight; ++y) {
        unsigned int *pPixel = reinterpret_cast<unsigned int *>(pRow);
        for (int x = 0; x < mBitmap.mWidth; ++x) {
            const unsigned int nChannels = *pPixel & kACanvas32ChannelsMask;
            if (nChannels == nColorKey) {
                *pPixel = nChannels;
            } else {
                *pPixel |= kACanvas32AlphaOpaque;
            }
            ++pPixel;
        }
        pRow += mBitmap.mBytesPerRow;
    }
}

// NTSC-U/C: 0x00614308, PAL: 0x00654e98
void ACanvasLin32::PutPixelNoClip(int nX, int nY) {
    *PixelAt(mBitmap, nX, nY) = mColor;
}

// NTSC-U/C: 0x00614330, PAL: 0x00654ec0
void ACanvasLin32::PutPixelNoClip(int nX, int nY, unsigned int nColor) {
    *PixelAt(mBitmap, nX, nY) = nColor;
}

// NTSC-U/C: 0x00614350, PAL: 0x00654ee0
unsigned int ACanvasLin32::GetPixelNoClip(int nX, int nY) {
    return *PixelAt(mBitmap, nX, nY);
}

// NTSC-U/C: 0x00614370, PAL: 0x00654f00
void ACanvasLin32::FillRowNoClip(int nY, int nLeft, int nRight) {
    unsigned int *pPixel = PixelAt(mBitmap, nLeft, nY);
    // Yes, the binary counts down to zero exactly. A reversed span runs on.
    for (int nRemaining = nRight - nLeft; nRemaining != 0; --nRemaining) {
        *pPixel++ = mColor;
    }
}

// NTSC-U/C: 0x006143b8, PAL: 0x00654f48
void ACanvasLin32::FillColumnNoClip(int nX, int nTop, int nBottom) {
    unsigned char *pPixel = ByteAt(mBitmap, nX, nTop);
    // Yes, the binary counts down to zero exactly. A reversed span runs on.
    for (int nRemaining = nBottom - nTop; nRemaining != 0; --nRemaining) {
        *reinterpret_cast<unsigned int *>(pPixel) = mColor;
        pPixel += mBitmap.mBytesPerRow;
    }
}

// NTSC-U/C: 0x00614408, PAL: 0x00654f98
void ACanvasLin32::FillRectNoClip(ARect rect) {
    const short nWidth = static_cast<short>(rect.mRight - rect.mLeft);
    unsigned char *pRow = ByteAt(mBitmap, rect.mLeft, rect.mTop);
    const int nGap = mBitmap.mBytesPerRow - kBytesPerPixel * nWidth;
    for (short nRemainingRows = static_cast<short>(rect.mBottom - rect.mTop); nRemainingRows > 0;
         --nRemainingRows) {
        unsigned int *pPixel = reinterpret_cast<unsigned int *>(pRow);
        // Yes, the binary counts the columns down to zero exactly. A reversed span runs on.
        for (int nRemaining = nWidth; nRemaining != 0; --nRemaining) {
            *pPixel++ = mColor;
        }
        pRow = reinterpret_cast<unsigned char *>(pPixel) + nGap;
    }
}

// NTSC-U/C: 0x006148e0, PAL: 0x00655470
void ACanvasLin32::TextureRowIndexed(int nY,
                                     int nLeft,
                                     int nRight,
                                     const ABitmap *pSource,
                                     APoint *pSourcePosition,
                                     const APoint *pSourceStep) {
    const APalette *pPalette = ResolvePalette(*pSource, mBitmap);
    if (pPalette == nullptr) {
        return;
    }
    unsigned int *pDest = PixelAt(mBitmap, nLeft, nY);
    for (int nRemaining = nRight - nLeft; nRemaining > 0; --nRemaining) {
        const unsigned char *pPixel =
            static_cast<const unsigned char *>(pSource->mPixels) +
            (pSourcePosition->mY >> kACanvasFractionBits) * pSource->mBytesPerRow +
            (pSourcePosition->mX >> kACanvasFractionBits);
        *pDest++ = pPalette->mEntries[*pPixel];
        pSourcePosition->mX += pSourceStep->mX;
        pSourcePosition->mY += pSourceStep->mY;
    }
}

// NTSC-U/C: 0x006144b0, PAL: 0x00655040
void ACanvasLin32::Blit4NoClip(const ABitmap &source, int nX, int nY) {
    const APalette *pPalette = ResolvePalette(source, mBitmap);
    if (pPalette == nullptr) {
        return;
    }
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    unsigned char *pDestRow = ByteAt(mBitmap, nX, nY);
    for (int nRemainingRows = source.mHeight; nRemainingRows > 0; --nRemainingRows) {
        UnpackNibbleRow(pSourceRow, g_abCanvasRowScratch, source.mWidth, source.mOddNibbleStart);
        const unsigned char *pIndex = g_abCanvasRowScratch;
        unsigned int *pDest = reinterpret_cast<unsigned int *>(pDestRow);
        for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
            if (source.mHasTransparentColor == 0 ||
                *pIndex != static_cast<unsigned char>(source.mTransparentColor)) {
                *pDest = pPalette->mEntries[*pIndex];
            }
            ++pIndex;
            ++pDest;
        }
        pSourceRow += source.mBytesPerRow;
        pDestRow += mBitmap.mBytesPerRow;
    }
}

// NTSC-U/C: 0x00614608, PAL: 0x00655198
void ACanvasLin32::Blit8NoClip(const ABitmap &source, int nX, int nY) {
    const APalette *pPalette = ResolvePalette(source, mBitmap);
    if (pPalette == nullptr) {
        return;
    }
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    unsigned char *pDestRow = ByteAt(mBitmap, nX, nY);
    for (int nRemainingRows = source.mHeight; nRemainingRows > 0; --nRemainingRows) {
        const unsigned char *pIndex = pSourceRow;
        unsigned int *pDest = reinterpret_cast<unsigned int *>(pDestRow);
        for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
            if (source.mHasTransparentColor == 0 ||
                *pIndex != static_cast<unsigned char>(source.mTransparentColor)) {
                *pDest = pPalette->mEntries[*pIndex];
            }
            ++pIndex;
            ++pDest;
        }
        pSourceRow += source.mBytesPerRow;
        pDestRow += mBitmap.mBytesPerRow;
    }
}

// NTSC-U/C: 0x00614070, PAL: 0x00654c00
void ACanvasLin32::Blit32NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    unsigned char *pDestRow = ByteAt(mBitmap, nX, nY);
    if (source.mHasTransparentColor == 0 && source.mBytesPerRow == mBitmap.mBytesPerRow) {
        memcpy(pDestRow, pSourceRow, source.mByteCount);
        return;
    }
    for (int nRemainingRows = source.mHeight; nRemainingRows > 0; --nRemainingRows) {
        if (source.mHasTransparentColor != 0) {
            const unsigned int *pSourcePixel = reinterpret_cast<const unsigned int *>(pSourceRow);
            unsigned int *pDest = reinterpret_cast<unsigned int *>(pDestRow);
            for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
                if (source.mTransparentColor != *pSourcePixel) {
                    *pDest = *pSourcePixel;
                }
                ++pSourcePixel;
                ++pDest;
            }
        } else {
            memcpy(pDestRow, pSourceRow, kBytesPerPixel * source.mWidth);
        }
        pSourceRow += source.mBytesPerRow;
        pDestRow += mBitmap.mBytesPerRow;
    }
}

// NTSC-U/C: 0x006146f0, PAL: 0x00655280
void ACanvasLin32::RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned int *pDest = PixelAt(mBitmap, span.mLeft, span.mY);
    const unsigned char *pIndex = span.mSource;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        if (!span.mHasTransparentColor ||
            *pIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            *pDest = span.mPalette->mEntries[pRemap[*pIndex]];
        }
        ++pIndex;
        ++pDest;
    }
}

// NTSC-U/C: 0x00614790, PAL: 0x00655320
void ACanvasLin32::StretchRowIndexed(const AStretchSpan &span) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned int *pDest = PixelAt(mBitmap, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            *pDest = span.mPalette->mEntries[nIndex];
        }
        ++pDest;
        nPosition += span.mSourceStep;
    }
}

// NTSC-U/C: 0x00614830, PAL: 0x006553c0
void ACanvasLin32::StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap) {
    if (span.mPalette == nullptr) {
        return;
    }
    unsigned int *pDest = PixelAt(mBitmap, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining > 0; --nRemaining) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            *pDest = span.mPalette->mEntries[pRemap[nIndex]];
        }
        ++pDest;
        nPosition += span.mSourceStep;
    }
}
