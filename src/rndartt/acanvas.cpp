#include "rndartt/acanvas.h"

#include <string.h>

#include "rndartt/afont.h"
#include "rndartt/apoint.h"
#include "rndartt/arowspan.h"
#include "rndartt/astretchspan.h"

namespace {

constexpr unsigned int kChannelMask = 0xff;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;
constexpr int kNibbleBits = 4;
constexpr unsigned int kNibbleMask = 0x0f;
constexpr int kRGBByteCount = 3;
constexpr char kNewline = '\n';

// One pointer to member per ABitmapFormat code. DrawGlyphNoClip() and DrawGlyph() index the first
// two tables by the glyph format code, and the third belongs to the read slots.
typedef void (ACanvas::*ABitmapCopyMember)(const ABitmap &, int, int);

// 0x0077dc98
const ABitmapCopyMember kCopyNoClipForFormat[kABitmapFormatCount] = {&ACanvas::Blit4NoClip,
                                                                     &ACanvas::Blit8NoClip,
                                                                     &ACanvas::Blit15NoClip,
                                                                     &ACanvas::Blit24NoClip,
                                                                     &ACanvas::Blit32NoClip,
                                                                     &ACanvas::BlitRle8NoClip};

// 0x0077dcc8
const ABitmapCopyMember kCopyForFormat[kABitmapFormatCount] = {&ACanvas::Blit4,
                                                               &ACanvas::Blit8,
                                                               &ACanvas::Blit15,
                                                               &ACanvas::Blit24,
                                                               &ACanvas::Blit32,
                                                               &ACanvas::BlitRle8};

inline const unsigned char *SourceRow(const ABitmap &bitmap) {
    return static_cast<const unsigned char *>(bitmap.mPixels);
}

inline unsigned char *DestRow(const ABitmap &bitmap) {
    return static_cast<unsigned char *>(bitmap.mPixels);
}

inline const ABitmap *GlyphForCode(const AFont *pFont, int nCharCode) {
    const int nIndex = nCharCode - pFont->mFirstCharCode;
    // Yes, the binary tests only the upper bound, so a code below mFirstCharCode indexes before
    // the array, and a code at or beyond the count yields a null the caller then reads through.
    if (nIndex < pFont->mGlyphCount) {
        return pFont->mGlyphs[nIndex];
    }
    return nullptr;
}

} // namespace

// 0x0086f6f0
APalette *g_pDefaultPalette = nullptr;

// 0x005eb1a0
ACanvas::ACanvas(const ABitmap &bitmap) : mBitmap(bitmap) {
    mClip.mLeft = 0;
    mClip.mTop = 0;
    mClip.mRight = bitmap.mWidth;
    mClip.mBottom = bitmap.mHeight;
}

// 0x005ead68
ACanvas::~ACanvas() {
}

// 0x005eb3d0
int ACanvas::ClipCodeForPoint(int nX, int nY) const {
    int nCode = 0;
    if (nX < mClip.mLeft) {
        nCode |= kACanvasClipLeft;
    }
    if (nX >= mClip.mRight) {
        nCode |= kACanvasClipRight;
    }
    if (nY < mClip.mTop) {
        nCode |= kACanvasClipAbove;
    }
    if (nY >= mClip.mBottom) {
        nCode |= kACanvasClipBelow;
    }
    return nCode;
}

// 0x005eb520
void ACanvas::PutPixel(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelNoClip(nX, nY);
    }
}

// 0x005eb590
void ACanvas::PutPixelIndexed(int nX, int nY, int nIndex) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelIndexedNoClip(nX, nY, nIndex & kChannelMask);
    }
}

// 0x005eb608
void ACanvas::PutPixel15(int nX, int nY, unsigned short nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixel15NoClip(nX, nY, nColor);
    }
}

// 0x005eb680
void ACanvas::PutPixelRGB(int nX, int nY, const unsigned char *pRGB) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelRGBNoClip(nX, nY, pRGB);
    }
}

// 0x005eb6f0
void ACanvas::PutPixel(int nX, int nY, unsigned int nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelNoClip(nX, nY, nColor);
    }
}

// 0x005eb760
void ACanvas::PutPixelNative(int nX, int nY, unsigned int nColor) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        PutPixelNativeNoClip(nX, nY, nColor);
    }
}

// 0x005eb7d0
int ACanvas::GetPixelIndexed(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixelIndexedNoClip(nX, nY);
    }
    return 0;
}

// 0x005eb848
unsigned short ACanvas::GetPixel15(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixel15NoClip(nX, nY);
    }
    return 0;
}

// 0x005eb8c0
void ACanvas::GetPixelRGB(int nX, int nY, unsigned char *pRGB) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        GetPixelRGBNoClip(nX, nY, pRGB);
        return;
    }
    memset(pRGB, 0, kRGBByteCount);
}

// 0x005eb948
unsigned int ACanvas::GetPixel(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixelNoClip(nX, nY);
    }
    return 0;
}

// 0x005eb9c0
unsigned int ACanvas::GetPixelNative(int nX, int nY) {
    if (nX >= mClip.mLeft && nX < mClip.mRight && nY >= mClip.mTop && nY < mClip.mBottom) {
        return GetPixelNativeNoClip(nX, nY);
    }
    return 0;
}

// 0x005eba38
void ACanvas::FillRowNoClip(int nY, int nLeft, int nRight) {
    for (int x = nLeft; x < nRight; ++x) {
        PutPixelNoClip(x, nY);
    }
}

// 0x005ebab8
void ACanvas::FillRow(int nY, int nLeft, int nRight) {
    if (nY < mClip.mTop || nY >= mClip.mBottom) {
        return;
    }
    if (nLeft < mClip.mLeft) {
        nLeft = mClip.mLeft;
    }
    if (mClip.mRight < nRight) {
        nRight = mClip.mRight;
    }
    if (nLeft < nRight) {
        FillRowNoClip(nY, nLeft, nRight);
    }
}

// 0x005ebb30
void ACanvas::FillColumnNoClip(int nX, int nTop, int nBottom) {
    for (int y = nTop; y < nBottom; ++y) {
        PutPixelNoClip(nX, y);
    }
}

// 0x005ebbb0
void ACanvas::FillColumn(int nX, int nTop, int nBottom) {
    if (nX < mClip.mLeft || nX >= mClip.mRight) {
        return;
    }
    if (nTop < mClip.mTop) {
        nTop = mClip.mTop;
    }
    if (mClip.mBottom < nBottom) {
        nBottom = mClip.mBottom;
    }
    if (nTop < nBottom) {
        FillColumnNoClip(nX, nTop, nBottom);
    }
}

// 0x005ebc28
void ACanvas::FillRectNoClip(ARect rect) {
    for (int y = rect.mTop; y < rect.mBottom; ++y) {
        FillRowNoClip(y, rect.mLeft, rect.mRight);
    }
}

// 0x005ebc98
void ACanvas::FillRect(ARect rect) {
    rect = rect.Intersection(mClip);
    if (rect.mLeft < rect.mRight && rect.mTop < rect.mBottom) {
        FillRectNoClip(rect);
    }
}

// 0x005e9378
void ACanvas::FrameRectNoClip(ARect rect) {
    FillRowNoClip(rect.mTop, rect.mLeft, rect.mRight);
    FillRowNoClip(rect.mBottom - 1, rect.mLeft, rect.mRight);
    FillColumnNoClip(rect.mLeft, rect.mTop + 1, rect.mBottom - 1);
    FillColumnNoClip(rect.mRight - 1, rect.mTop + 1, rect.mBottom - 1);
}

// 0x005e9440
void ACanvas::FrameRect(ARect rect) {
    FillRow(rect.mTop, rect.mLeft, rect.mRight);
    FillRow(rect.mBottom - 1, rect.mLeft, rect.mRight);
    FillColumn(rect.mLeft, rect.mTop + 1, rect.mBottom - 1);
    FillColumn(rect.mRight - 1, rect.mTop + 1, rect.mBottom - 1);
}

// 0x005ebd40
void ACanvas::RemapRectIndices(ARect rect, const unsigned char *pRemap) {
    for (int y = rect.mTop; y < rect.mBottom; ++y) {
        for (int x = rect.mLeft; x < rect.mRight; ++x) {
            PutPixelIndexedNoClip(x, y, pRemap[GetPixelIndexedNoClip(x, y)]);
        }
    }
}

// 0x005ebec8
void ACanvas::DrawLineNoClip(int nX0, int nY0, int nX1, int nY1) {
    int nStepX = 0;
    int nStepY = 0;
    for (int nRemaining = SetupLineSteps(nX0, nY0, nX1, nY1, &nStepX, &nStepY); nRemaining > 0;
         --nRemaining) {
        PutPixelNoClip(nX0 >> kACanvasFractionBits, nY0 >> kACanvasFractionBits);
        nX0 += nStepX;
        nY0 += nStepY;
    }
}

// 0x005ebf70
void ACanvas::DrawLine(int nX0, int nY0, int nX1, int nY1) {
    if (ClipLineToRect(&nX0, &nY0, &nX1, &nY1) != 0) {
        DrawLineNoClip(nX0, nY0, nX1, nY1);
    }
}

// 0x005ec050
void ACanvas::TextureRowIndexed(int nY,
                                int nLeft,
                                int nRight,
                                const ABitmap *pSource,
                                APoint *pSourcePosition,
                                const APoint *pSourceStep) {
    for (int x = nLeft; x < nRight; ++x) {
        const unsigned char *pPixel =
            SourceRow(*pSource) +
            (pSourcePosition->mY >> kACanvasFractionBits) * pSource->mBytesPerRow +
            (pSourcePosition->mX >> kACanvasFractionBits);
        PutPixelIndexedNoClip(x, nY, *pPixel);
        pSourcePosition->mX += pSourceStep->mX;
        pSourcePosition->mY += pSourceStep->mY;
    }
}

// 0x005ec280
void ACanvas::Blit4NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pRow = SourceRow(source);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        const unsigned char *pByte = pRow;
        unsigned int bHighNibble = source.mOddNibbleStart;
        for (int x = nX; x < nX + source.mWidth; ++x) {
            const int nIndex = bHighNibble != 0 ? (*pByte >> kNibbleBits) : (*pByte & kNibbleMask);
            const unsigned char *pNext = pByte + 1;
            if (bHighNibble != 0) {
                pByte = pNext;
            }
            bHighNibble ^= 1;
            // Yes, the test compares a whole source byte against the key rather than the nibble
            // just consumed, and it reads the byte the walk has already advanced to.
            if (source.mHasTransparentColor == 0 || *pByte != source.mTransparentColor) {
                PutPixelIndexedNoClip(x, y, nIndex);
            }
        }
        pRow += source.mBytesPerRow;
    }
}

// 0x005ec3c0
void ACanvas::Blit4(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit4NoClip(clipped, nX, nY);
    }
}

// 0x005ec450
void ACanvas::Blit8NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pPixel = SourceRow(source);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            if (source.mHasTransparentColor == 0 || *pPixel != source.mTransparentColor) {
                PutPixelIndexedNoClip(x, y, *pPixel);
            }
            ++pPixel;
        }
        pPixel += source.mBytesPerRow - source.mWidth;
    }
}

// 0x005ec570
void ACanvas::Blit8(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit8NoClip(clipped, nX, nY);
    }
}

// 0x005ec600
void ACanvas::Blit15NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned short *pPixel = static_cast<const unsigned short *>(source.mPixels);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            if (source.mHasTransparentColor == 0 || *pPixel != source.mTransparentColor) {
                PutPixel15NoClip(x, y, *pPixel);
            }
            ++pPixel;
        }
        pPixel += source.mBytesPerRow / 2 - source.mWidth;
    }
}

// 0x005ec738
void ACanvas::Blit15(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit15NoClip(clipped, nX, nY);
    }
}

// 0x005ec7c8
void ACanvas::Blit24NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pPixel = SourceRow(source);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            bool bDraw = true;
            if (source.mHasTransparentColor != 0) {
                unsigned char abPixel[4];
                abPixel[0] = pPixel[0];
                abPixel[1] = pPixel[1];
                abPixel[2] = pPixel[2];
                abPixel[3] = 0;
                bDraw = source.mTransparentColor != *reinterpret_cast<unsigned int *>(abPixel);
            }
            if (bDraw) {
                PutPixelRGBNoClip(x, y, pPixel);
            }
            pPixel += kRGBByteCount;
        }
        pPixel += source.mBytesPerRow - kRGBByteCount * source.mWidth;
    }
}

// 0x005ec928
void ACanvas::Blit24(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit24NoClip(clipped, nX, nY);
    }
}

// 0x005ec9b8
void ACanvas::Blit32NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned int *pPixel = static_cast<const unsigned int *>(source.mPixels);
    for (int y = nY; y < nY + source.mHeight; ++y) {
        for (int x = nX; x < nX + source.mWidth; ++x) {
            if (source.mHasTransparentColor == 0 || source.mTransparentColor != *pPixel) {
                PutPixelNoClip(x, y, *pPixel);
            }
            ++pPixel;
        }
        pPixel += source.mBytesPerRow / 4 - source.mWidth;
    }
}

// 0x005ecad8
void ACanvas::Blit32(const ABitmap &source, int nX, int nY) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        Blit32NoClip(clipped, nX, nY);
    }
}

// 0x005ecb68
void ACanvas::BlitRle8NoClip(const ABitmap &source, int nX, int nY) {
    ABitmap row(g_abCanvasRowScratch,
                kABitmapFormatLinear8,
                source.mHasTransparentColor != 0,
                source.mWidth,
                1,
                0);
    row.mTransparentColor = source.mTransparentColor;
    row.mPalette = source.mPalette;
    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;
    for (int y = nY; y < nY + source.mHeight; ++y) {
        reader.DecodeRow(g_abCanvasRowScratch);
        Blit8NoClip(row, nX, y);
    }
}

// 0x005e9cb8
void ACanvas::BlitRle8(const ABitmap &source, int nX, int nY) {
    if (nY >= mClip.mTop && mClip.mBottom >= nY + source.mHeight && nX >= mClip.mLeft &&
        mClip.mRight >= nX + source.mWidth) {
        BlitRle8NoClip(source, nX, nY);
        return;
    }

    ARleReader reader;
    reader.mSource = SourceRow(source);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = kARleReaderNoTransparentValue;

    short nSkipLeft = 0;
    short nStopColumn = source.mWidth;
    if (mClip.mRight < nX + source.mWidth) {
        nStopColumn = static_cast<short>(mClip.mRight - nX);
    }
    if (nX < mClip.mLeft) {
        nSkipLeft = static_cast<short>(mClip.mLeft - nX);
        nX = mClip.mLeft;
    }
    if (nSkipLeft >= nStopColumn) {
        return;
    }

    short nStopRow = static_cast<short>(nY + source.mHeight);
    if (mClip.mBottom < nStopRow) {
        nStopRow = mClip.mBottom;
    }
    if (nY < mClip.mTop) {
        reader.SkipRows(mClip.mTop - nY);
        nY = mClip.mTop;
    }
    if (nY >= nStopRow) {
        return;
    }

    ABitmap row(g_abCanvasRowScratch,
                kABitmapFormatLinear8,
                source.mHasTransparentColor != 0,
                source.mWidth,
                1,
                0);
    row.mPixels = g_abCanvasRowScratch + nSkipLeft;
    row.mWidth = static_cast<short>(nStopColumn - nSkipLeft);
    row.mTransparentColor = source.mTransparentColor;
    row.mPalette = source.mPalette;
    for (int y = nY; y < nStopRow; ++y) {
        reader.DecodeRow(g_abCanvasRowScratch);
        Blit8NoClip(row, nX, y);
    }
}

// 0x005ecef0
void ACanvas::ReadRect4(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect4NoClip(clipped, nX, nY);
    }
}

// 0x005ecdb8
void ACanvas::ReadRect4NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned char *pRow = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        unsigned char *pByte = pRow;
        unsigned int bHighNibble = dest.mOddNibbleStart;
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            const int nIndex = GetPixelIndexedNoClip(x, y);
            if (bHighNibble != 0) {
                *pByte =
                    static_cast<unsigned char>((*pByte & kNibbleMask) | (nIndex << kNibbleBits));
                ++pByte;
            } else {
                *pByte = static_cast<unsigned char>(nIndex | (*pByte & ~kNibbleMask));
            }
            bHighNibble ^= 1;
        }
        pRow += dest.mBytesPerRow;
    }
}

// 0x005ed070
void ACanvas::ReadRect8(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect8NoClip(clipped, nX, nY);
    }
}

// 0x005ecf80
void ACanvas::ReadRect8NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned char *pPixel = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = static_cast<unsigned char>(GetPixelIndexedNoClip(x, y));
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow - dest.mWidth;
    }
}

// 0x005ed208
void ACanvas::ReadRect15(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect15NoClip(clipped, nX, nY);
    }
}

// 0x005ed100
void ACanvas::ReadRect15NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned short *pPixel = static_cast<unsigned short *>(dest.mPixels);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = GetPixel15NoClip(x, y);
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow / 2 - dest.mWidth;
    }
}

// 0x005ed398
void ACanvas::ReadRect24(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect24NoClip(clipped, nX, nY);
    }
}

// 0x005ed298
void ACanvas::ReadRect24NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned char *pPixel = DestRow(dest);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            GetPixelRGBNoClip(x, y, pPixel);
            pPixel += kRGBByteCount;
        }
        pPixel += dest.mBytesPerRow - kRGBByteCount * dest.mWidth;
    }
}

// 0x005ed520
void ACanvas::ReadRect32(const ABitmap &dest, int nX, int nY) {
    ABitmap clipped = dest;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        ReadRect32NoClip(clipped, nX, nY);
    }
}

// 0x005ed428
void ACanvas::ReadRect32NoClip(const ABitmap &dest, int nX, int nY) {
    unsigned int *pPixel = static_cast<unsigned int *>(dest.mPixels);
    for (int y = nY; y < nY + dest.mHeight; ++y) {
        for (int x = nX; x < nX + dest.mWidth; ++x) {
            *pPixel = GetPixelNoClip(x, y);
            ++pPixel;
        }
        pPixel += dest.mBytesPerRow / 4 - dest.mWidth;
    }
}

// 0x005e9ee8
void ACanvas::DrawGlyphNoClip(int nCharCode, const AFont *pFont, int nX, int nY) {
    const ABitmap *pGlyph = GlyphForCode(pFont, nCharCode);
    (this->*kCopyNoClipForFormat[pGlyph->mFormat])(*pGlyph, nX, nY - pFont->mBaseline);
}

// 0x005e9fc8
void ACanvas::DrawGlyph(int nCharCode, const AFont *pFont, int nX, int nY) {
    const ABitmap *pGlyph = GlyphForCode(pFont, nCharCode);
    // The compiled copy is 0x1c bytes, four more than an ABitmap. See the note in abitmap.h.
    ABitmap clipped = *pGlyph;
    nY -= pFont->mBaseline;
    if (ClipBlitToRect(&clipped, &nX, &nY) != 0) {
        (this->*kCopyForFormat[clipped.mFormat])(clipped, nX, nY);
    }
}

// 0x005ed5b0
void ACanvas::DrawTextNoClip(const char *pText, const AFont *pFont, int nX, int nY) {
    const int nStartX = nX;
    for (unsigned char ch = *pText++; ch != 0; ch = *pText++) {
        if (ch == kNewline) {
            nY += pFont->mLineHeight;
            nX = nStartX;
            continue;
        }
        DrawGlyphNoClip(ch, pFont, nX, nY);
        nX += GlyphForCode(pFont, ch)->mWidth;
    }
}

// 0x005ea120
void ACanvas::DrawText(const char *pText, const AFont *pFont, int nX, int nY) {
    if (nY - pFont->mBaseline >= mClip.mBottom) {
        return;
    }
    if (nY + pFont->mLineHeight - pFont->mBaseline < mClip.mTop) {
        // Yes, the binary walks to the end of the string and then returns without drawing.
        while (*pText != '\0') {
            ++pText;
        }
        return;
    }
    const int nStartX = nX;
    for (unsigned char ch = *pText++; ch != 0; ch = *pText++) {
        if (ch == kNewline) {
            nY += pFont->mLineHeight;
            if (nY - pFont->mBaseline >= mClip.mBottom) {
                return;
            }
            nX = nStartX;
            continue;
        }
        DrawGlyph(ch, pFont, nX, nY);
        nX += GlyphForCode(pFont, ch)->mWidth;
    }
}

// 0x005ed858
void ACanvas::BlitRemap4(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = g_abCanvasRowScratch;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    const unsigned char *pRow = SourceRow(source);
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        UnpackNibbleRow(pRow, g_abCanvasRowScratch, source.mWidth, source.mOddNibbleStart);
        RemapRowIndexed(span, pRemap);
        pRow += source.mBytesPerRow;
    }
}

// 0x005eda30
void ACanvas::BlitRemap8(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = const_cast<unsigned char *>(SourceRow(source));
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        RemapRowIndexed(span, pRemap);
        span.mSource += source.mBytesPerRow;
    }
}

// 0x005edd08
void ACanvas::RemapRowIndexed(const ARowSpan &span, const unsigned char *pRemap) {
    const unsigned char *pPixel = span.mSource;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nIndex = *pPixel++;
        if (!span.mHasTransparentColor || nIndex != span.mTransparentColor) {
            PutPixelIndexedNoClip(x, span.mY, pRemap[nIndex]);
        }
    }
}

// 0x005edfb8
void ACanvas::BlitBlend4(const ABitmap &source,
                         int nX,
                         int nY,
                         const unsigned char *const *ppBlend) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = g_abCanvasRowScratch;
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    const unsigned char *pRow = SourceRow(source);
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        UnpackNibbleRow(pRow, g_abCanvasRowScratch, source.mWidth, source.mOddNibbleStart);
        BlendRowIndexed(span, ppBlend);
        pRow += source.mBytesPerRow;
    }
}

// 0x005ee1c8
void ACanvas::BlitBlend8(const ABitmap &source,
                         int nX,
                         int nY,
                         const unsigned char *const *ppBlend) {
    ARowSpan span;
    span.mLeft = static_cast<short>(nX);
    span.mRight = static_cast<short>(source.mWidth + nX);
    span.mHasTransparentColor = source.mHasTransparentColor != 0;
    span.mTransparentColor = source.mTransparentColor;
    span.mSource = const_cast<unsigned char *>(SourceRow(source));
    span.mPalette = source.mPalette;
    if (span.mPalette == nullptr) {
        span.mPalette = mBitmap.mPalette;
        if (span.mPalette == nullptr) {
            span.mPalette = g_pDefaultPalette;
        }
    }
    for (span.mY = static_cast<short>(nY); span.mY < nY + source.mHeight; ++span.mY) {
        BlendRowIndexed(span, ppBlend);
        span.mSource += source.mBytesPerRow;
    }
}

// 0x005ee4a0
void ACanvas::BlendRowIndexed(const ARowSpan &span, const unsigned char *const *ppBlend) {
    const unsigned char *pPixel = span.mSource;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nIndex = *pPixel++;
        // Index zero is the transparent one here, unlike RemapRowIndexed(), which compares
        // against the span transparent colour.
        if (nIndex == 0 && span.mHasTransparentColor) {
            continue;
        }
        const int nDestIndex = GetPixelIndexedNoClip(x, span.mY);
        PutPixelIndexedNoClip(x, span.mY, ppBlend[nIndex][nDestIndex]);
    }
}

// 0x005ee968
void ACanvas::StretchRowIndexed(const AStretchSpan &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 || nIndex != span.mTransparentColor) {
            PutPixelIndexedNoClip(x, span.mY, nIndex);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eea18
void ACanvas::StretchRow15(const AStretchSpan &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned short nColor = *reinterpret_cast<const unsigned short *>(
            span.mSource + 2 * (nPosition >> kACanvasFractionBits));
        if (span.mHasTransparentColor == 0 || nColor != span.mTransparentColor) {
            PutPixel15NoClip(x, span.mY, nColor);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eeac8
void ACanvas::StretchRow24(const AStretchSpan &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char *pRGB =
            span.mSource + kRGBByteCount * (nPosition >> kACanvasFractionBits);
        bool bDraw = true;
        if (span.mHasTransparentColor != 0) {
            unsigned char abPixel[4];
            abPixel[0] = pRGB[0];
            abPixel[1] = pRGB[1];
            abPixel[2] = pRGB[2];
            abPixel[3] = 0;
            bDraw = span.mTransparentColor != *reinterpret_cast<unsigned int *>(abPixel);
        }
        if (bDraw) {
            PutPixelRGBNoClip(x, span.mY, pRGB);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eebb8
void ACanvas::StretchRow32(const AStretchSpan &span) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned int nColor = *reinterpret_cast<const unsigned int *>(
            span.mSource + 4 * (nPosition >> kACanvasFractionBits));
        if (span.mHasTransparentColor == 0 || span.mTransparentColor != nColor) {
            PutPixelNoClip(x, span.mY, nColor);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eedb8
void ACanvas::StretchRowRemap(const AStretchSpan &span, const unsigned char *pRemap) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 || nIndex != span.mTransparentColor) {
            PutPixelIndexedNoClip(x, span.mY, pRemap[nIndex]);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eefc0
void ACanvas::StretchRowBlend(const AStretchSpan &span, const unsigned char *const *ppBlend) {
    int nPosition = span.mSourcePosition;
    for (int x = span.mLeft; x < span.mRight; ++x) {
        const unsigned char nIndex = span.mSource[nPosition >> kACanvasFractionBits];
        if (span.mHasTransparentColor == 0 || nIndex != span.mTransparentColor) {
            const int nDestIndex = GetPixelIndexedNoClip(x, span.mY);
            PutPixelIndexedNoClip(x, span.mY, ppBlend[nIndex][nDestIndex]);
        }
        nPosition += span.mSourceStep;
    }
}

// 0x005eddb8
void ACanvas::UnpackNibbleRow(const unsigned char *pSource,
                              unsigned char *pDest,
                              int nCount,
                              int bStartHighNibble) {
    while (nCount-- > 0) {
        if (bStartHighNibble != 0) {
            *pDest = static_cast<unsigned char>(*pSource++ >> kNibbleBits);
        } else {
            *pDest = static_cast<unsigned char>(*pSource & kNibbleMask);
        }
        ++pDest;
        bStartHighNibble ^= 1;
    }
}

// 0x005ed6a8. A format the chain does not test draws nothing at all, rather than falling back to a
// generic path.
void ACanvas::BlitRemapNoClip(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        BlitRemap4(source, nX, nY, pRemap);
        break;
    case kABitmapFormatLinear8:
        BlitRemap8(source, nX, nY, pRemap);
        break;
    case kABitmapFormatRle8:
        BlitRemapRle8NoClip(source, nX, nY, pRemap);
        break;
    default:
        break;
    }
}

// 0x005ed718. The description is copied before clipping, because ClipBlitToRect() rewrites the one
// it is given.
void ACanvas::BlitRemap(const ABitmap &source, int nX, int nY, const unsigned char *pRemap) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) == 0) {
        return;
    }
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        BlitRemap4(clipped, nX, nY, pRemap);
        break;
    case kABitmapFormatLinear8:
        BlitRemap8(clipped, nX, nY, pRemap);
        break;
    case kABitmapFormatRle8:
        BlitRemapRle8(clipped, nX, nY, pRemap);
        break;
    default:
        break;
    }
}

// 0x005ede08
void ACanvas::BlitBlendNoClip(const ABitmap &source,
                              int nX,
                              int nY,
                              const unsigned char *const *ppBlend) {
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        BlitBlend4(source, nX, nY, ppBlend);
        break;
    case kABitmapFormatLinear8:
        BlitBlend8(source, nX, nY, ppBlend);
        break;
    case kABitmapFormatRle8:
        BlitBlendRle8NoClip(source, nX, nY, ppBlend);
        break;
    default:
        break;
    }
}

// 0x005ede78
void ACanvas::BlitBlend(const ABitmap &source,
                        int nX,
                        int nY,
                        const unsigned char *const *ppBlend) {
    ABitmap clipped = source;
    if (ClipBlitToRect(&clipped, &nX, &nY) == 0) {
        return;
    }
    switch (source.mFormat) {
    case kABitmapFormatLinear4:
        BlitBlend4(clipped, nX, nY, ppBlend);
        break;
    case kABitmapFormatLinear8:
        BlitBlend8(clipped, nX, nY, ppBlend);
        break;
    case kABitmapFormatRle8:
        BlitBlendRle8(clipped, nX, nY, ppBlend);
        break;
    default:
        break;
    }
}
