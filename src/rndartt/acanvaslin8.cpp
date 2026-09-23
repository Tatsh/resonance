#include "rndartt/acanvaslin8.h"

#include <string.h>

#include "rndartt/apoint.h"
#include "rndartt/arlereader.h"
#include "rndartt/arowspan.h"
#include "rndartt/astretchspan.h"

namespace {

constexpr unsigned char kNibbleMask = 0x0f;
constexpr int kNibbleBits = 4;

// One byte is one pixel, so a pixel address needs no packing and no odd-start flag.
inline unsigned char *PixelAt(void *pPixels, int nBytesPerRow, int nX, int nY) {
    return static_cast<unsigned char *>(pPixels) + (nY * nBytesPerRow) + nX;
}

} // namespace

// 0x00628600
ACanvasLin8::ACanvasLin8(const ABitmap &bitmap) : ACanvas8(bitmap) {
    mColor = 0;
}

// 0x00628568
ACanvasLin8::~ACanvasLin8() {
}

// 0x00628638
void ACanvasLin8::PutPixelNoClip(int nX, int nY) {
    *PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY) = mColor;
}

// 0x00628658
void ACanvasLin8::PutPixelIndexedNoClip(int nX, int nY, int nIndex) {
    *PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY) = static_cast<unsigned char>(nIndex);
}

// 0x00628678
int ACanvasLin8::GetPixelIndexedNoClip(int nX, int nY) {
    return *PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
}

// 0x00628698
void ACanvasLin8::FillRowNoClip(int nY, int nLeft, int nRight) {
    memset(PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nLeft, nY),
           mColor,
           static_cast<unsigned int>(nRight - nLeft));
}

// 0x006286d0. The colour and the row pitch are both re-read on every iteration rather than
// hoisted, which is what the binary does.
void ACanvasLin8::FillColumnNoClip(int nX, int nTop, int nBottom) {
    if (nTop >= nBottom) {
        return;
    }
    unsigned char *pByte = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nTop);
    for (int nRow = nBottom - nTop; nRow != 0; --nRow) {
        *pByte = mColor;
        pByte += mBitmap.mBytesPerRow;
    }
}

// 0x00628718. One memset per row, with the rectangle's own bounds re-read each time round.
void ACanvasLin8::FillRectNoClip(ARect rect) {
    unsigned char *pRow = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, rect.mLeft, rect.mTop);
    for (int nRow = rect.mTop; nRow < rect.mBottom; ++nRow) {
        memset(pRow, mColor, static_cast<unsigned int>(rect.mRight - rect.mLeft));
        pRow += mBitmap.mBytesPerRow;
    }
}

// 0x006287b8. The row advance subtracts the column span whether or not the span was walked, so a
// rectangle whose right edge is left of its left edge advances by more than the pitch.
void ACanvasLin8::RemapRectIndices(ARect rect, const unsigned char *pRemap) {
    const int nColumns = rect.mRight - rect.mLeft;
    unsigned char *pByte = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, rect.mLeft, rect.mTop);
    for (int nRow = rect.mTop; nRow < rect.mBottom; ++nRow) {
        for (int nRemaining = nColumns; nRemaining > 0; --nRemaining) {
            *pByte = pRemap[*pByte];
            ++pByte;
        }
        pByte += mBitmap.mBytesPerRow - nColumns;
    }
}

// 0x00628848. The nibble phase starts from the source's odd-start flag and alternates per pixel,
// and the low nibble of a byte is the earlier pixel.
void ACanvasLin8::Blit4NoClip(const ABitmap &source, int nX, int nY) {
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    for (int nRow = source.mHeight; nRow > 0; --nRow) {
        const unsigned char *pSourceByte = pSourceRow;
        unsigned int nOddNibble = source.mOddNibbleStart;
        for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
            unsigned char nIndex = 0;
            if (nOddNibble == 0) {
                nIndex = *pSourceByte & kNibbleMask;
            } else {
                nIndex = static_cast<unsigned char>(*pSourceByte >> kNibbleBits);
                ++pSourceByte;
            }
            if (!source.mHasTransparentColor ||
                nIndex != static_cast<unsigned char>(source.mTransparentColor)) {
                *pDest = nIndex;
            }
            nOddNibble ^= 1;
            ++pDest;
        }
        pDest += mBitmap.mBytesPerRow - source.mWidth;
        pSourceRow += source.mBytesPerRow;
    }
}

// 0x00628918. Three tiers, widest first: a source with no transparent colour whose pitch matches
// this canvas's is one memcpy of the whole rectangle, a source with no transparent colour is one
// memcpy per row, and a keyed source is walked a byte at a time. The transparency test is repeated
// inside the row loop even though the flag cannot change between rows.
void ACanvasLin8::Blit8NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pSourceByte = static_cast<const unsigned char *>(source.mPixels);
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    if (!source.mHasTransparentColor && source.mBytesPerRow == mBitmap.mBytesPerRow) {
        memcpy(pDest, pSourceByte, static_cast<unsigned int>(source.mByteCount));
        return;
    }
    for (int nRow = source.mHeight; nRow > 0; --nRow) {
        if (source.mHasTransparentColor) {
            const unsigned char nKey = static_cast<unsigned char>(source.mTransparentColor);
            for (int nRemaining = source.mWidth; nRemaining > 0; --nRemaining) {
                if (*pSourceByte != nKey) {
                    *pDest = *pSourceByte;
                }
                ++pSourceByte;
                ++pDest;
            }
            pSourceByte += source.mBytesPerRow - source.mWidth;
            pDest += mBitmap.mBytesPerRow - source.mWidth;
        } else {
            memcpy(pDest, pSourceByte, static_cast<unsigned int>(source.mWidth));
            pSourceByte += source.mBytesPerRow;
            pDest += mBitmap.mBytesPerRow;
        }
    }
}

// 0x00628a48. The reader is built on the stack and the row decoder advances its cursor, so the
// rows are consumed in order without this routine tracking the compressed stream.
void ACanvasLin8::BlitRle8NoClip(const ABitmap &source, int nX, int nY) {
    ARleReader reader;
    reader.mSource = static_cast<const unsigned char *>(source.mPixels);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = source.mHasTransparentColor ?
                                   static_cast<int>(source.mTransparentColor) :
                                   kARleReaderNoTransparentValue;
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    for (int nRow = nY; nRow < nY + source.mHeight; ++nRow) {
        (void)reader.DecodeRow(pDest); // The advanced destination the decoder returns is discarded.
        pDest += mBitmap.mBytesPerRow;
    }
}

// 0x00628db0. The source position is advanced in place, so the caller sees where the row ended.
void ACanvasLin8::TextureRowIndexed(int nY,
                                    int nLeft,
                                    int nRight,
                                    const ABitmap *pSource,
                                    APoint *pSourcePosition,
                                    const APoint *pSourceStep) {
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nLeft, nY);
    for (int nRemaining = nRight - nLeft; nRemaining > 0; --nRemaining) {
        *pDest = *PixelAt(pSource->mPixels,
                          pSource->mBytesPerRow,
                          pSourcePosition->mX >> kACanvasFractionBits,
                          pSourcePosition->mY >> kACanvasFractionBits);
        ++pDest;
        pSourcePosition->mX += pSourceStep->mX;
        pSourcePosition->mY += pSourceStep->mY;
    }
}

// 0x00628ae8. The keyed and the opaque walks are separate loops in the binary.
void ACanvasLin8::RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap) {
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    const unsigned char *pSourceByte = span.mSource;
    const int nColumns = span.mRight - span.mLeft;
    if (span.mHasTransparentColor) {
        for (int nRemaining = nColumns; nRemaining > 0; --nRemaining) {
            if (*pSourceByte != static_cast<unsigned char>(span.mTransparentColor)) {
                *pDest = pRemap[*pSourceByte];
            }
            ++pSourceByte;
            ++pDest;
        }
        return;
    }
    for (int nRemaining = nColumns; nRemaining > 0; --nRemaining) {
        *pDest = pRemap[*pSourceByte];
        ++pSourceByte;
        ++pDest;
    }
}

// 0x00628ba8. Unlike ACanvas::BlendRowIndexed(), the key here is the span transparent colour
// rather than index zero.
void ACanvasLin8::BlendRowIndexed(const ARowSpan &span, const unsigned char *const *ppBlend) {
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    const unsigned char *pSourceByte = span.mSource;
    const int nColumns = span.mRight - span.mLeft;
    if (span.mHasTransparentColor) {
        for (int nRemaining = nColumns; nRemaining > 0; --nRemaining) {
            if (*pSourceByte != static_cast<unsigned char>(span.mTransparentColor)) {
                *pDest = ppBlend[*pSourceByte][*pDest];
            }
            ++pSourceByte;
            ++pDest;
        }
        return;
    }
    for (int nRemaining = nColumns; nRemaining > 0; --nRemaining) {
        *pDest = ppBlend[*pSourceByte][*pDest];
        ++pSourceByte;
        ++pDest;
    }
}

// 0x006284a0. The keyed loop runs until its counter passes zero exactly, where the opaque loop
// stops at any count of zero or less, so a reversed span walks far past the row only when keyed.
void ACanvasLin8::StretchRowIndexed(const AStretchSpan &span) {
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    const int nColumns = span.mRight - span.mLeft;
    if (span.mHasTransparentColor != 0) {
        for (int nRemaining = nColumns; nRemaining != 0; --nRemaining) {
            const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
            if (nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
                *pDest = nIndex;
            }
            ++pDest;
            nPosition += span.mSourceStep;
        }
        return;
    }
    for (int nRemaining = nColumns; nRemaining > 0; --nRemaining) {
        *pDest = span.mSource[nPosition >> kACanvasFractionBits];
        ++pDest;
        nPosition += span.mSourceStep;
    }
}

// 0x00628c80. The transparency flag is re-read for every pixel.
void ACanvasLin8::StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap) {
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining != 0; --nRemaining) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            *pDest = pRemap[nIndex];
        }
        ++pDest;
        nPosition += span.mSourceStep;
    }
}

// 0x00628d10
void ACanvasLin8::StretchRowBlend(const AStretchSpan &span, const unsigned char *const *ppBlend) {
    unsigned char *pDest = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, span.mLeft, span.mY);
    int nPosition = span.mSourcePosition;
    for (int nRemaining = span.mRight - span.mLeft; nRemaining != 0; --nRemaining) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 ||
            nIndex != static_cast<unsigned char>(span.mTransparentColor)) {
            *pDest = ppBlend[nIndex][*pDest];
        }
        ++pDest;
        nPosition += span.mSourceStep;
    }
}
