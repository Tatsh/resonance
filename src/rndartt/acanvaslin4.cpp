#include "rndartt/acanvaslin4.h"

#include <string.h>

#include "rndartt/arle8reader.h"

namespace {

constexpr unsigned int kNibbleMask = 0xf;
constexpr int kNibbleShift = 4;
constexpr int kPixelsPerByte = 2;

} // namespace

ACanvasLin4::ACanvasLin4(const ABitmap &bitmap) : ACanvas8(bitmap) {
    mColor = 0;
}

ACanvasLin4::~ACanvasLin4() {
}

void ACanvasLin4::DrawPixelU(int nX, int nY) {
    unsigned char *pByte = static_cast<unsigned char *>(mBitmap.mPixels) +
                           (nY * mBitmap.mBytesPerRow) + ((nX + mBitmap.mOddNibbleStart) / 2);
    if (((nX ^ mBitmap.mOddNibbleStart) & 1) != 0) {
        *pByte = static_cast<unsigned char>((*pByte & kNibbleMask) | (mColor << kNibbleShift));
    } else {
        // Yes, the whole colour index is OR'd in. Bits above the nibble spill into the odd pixel.
        *pByte = static_cast<unsigned char>((*pByte & (kNibbleMask << kNibbleShift)) | mColor);
    }
}

void ACanvasLin4::DrawPixel8U(int nX, int nY, int nIndex) {
    const unsigned int nValue = static_cast<unsigned int>(nIndex) & 0xff;
    unsigned char *pByte = static_cast<unsigned char *>(mBitmap.mPixels) +
                           (nY * mBitmap.mBytesPerRow) + ((nX + mBitmap.mOddNibbleStart) / 2);
    if (((nX ^ mBitmap.mOddNibbleStart) & 1) != 0) {
        *pByte = static_cast<unsigned char>((*pByte & kNibbleMask) | (nValue << kNibbleShift));
    } else {
        // Yes, the whole index byte is OR'd in, as in DrawPixelU().
        *pByte = static_cast<unsigned char>((*pByte & (kNibbleMask << kNibbleShift)) | nValue);
    }
}

int ACanvasLin4::GetPixel8U(int nX, int nY) {
    const unsigned char *pByte = static_cast<const unsigned char *>(mBitmap.mPixels) +
                                 (nY * mBitmap.mBytesPerRow) + ((nX + mBitmap.mOddNibbleStart) / 2);
    if (((nX ^ mBitmap.mOddNibbleStart) & 1) != 0) {
        return *pByte >> kNibbleShift;
    }
    return *pByte & kNibbleMask;
}

void ACanvasLin4::DrawBitmapLin4U(const ABitmap &source, int nX, int nY) {
    // The block copy requires five conditions. The source has no transparent colour, neither
    // bitmap starts on an odd nibble, and the destination column and the source width are both
    // even. Any one of them failing drops to unpacking each row into the shared scratch row and
    // writing it back a pixel at a time.
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    const bool bAligned = !source.mHasTransparentColor && mBitmap.mOddNibbleStart == 0 &&
                          source.mOddNibbleStart == 0 && (nX & 1) == 0 && (source.mWidth & 1) == 0;
    if (bAligned) {
        unsigned char *pDest = static_cast<unsigned char *>(mBitmap.mPixels) +
                               (nY * mBitmap.mBytesPerRow) +
                               ((nX + mBitmap.mOddNibbleStart) / kPixelsPerByte);
        // Yes, the binary counts down to zero exactly. A negative height runs on.
        for (int nRow = source.mHeight; nRow != 0; --nRow) {
            memcpy(pDest, pSourceRow, static_cast<unsigned int>(source.mWidth / kPixelsPerByte));
            pDest += mBitmap.mBytesPerRow;
            pSourceRow += source.mBytesPerRow;
        }
        return;
    }
    for (int nRow = 0; nRow < source.mHeight; ++nRow) {
        Unpack4(pSourceRow, ACanvas::tempBuff, source.mWidth, source.mOddNibbleStart);
        DrawBitmapRowLin8U(&source, ACanvas::tempBuff, nX, nY + nRow);
        pSourceRow += source.mBytesPerRow;
    }
}

void ACanvasLin4::DrawBitmapLin8U(const ABitmap &source, int nX, int nY) {
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    for (int nRow = nY; nRow < nY + source.mHeight; nRow += 2) {
        DrawBitmapRowLin8U(&source, pSourceRow, nX, nRow);
        pSourceRow += source.mBytesPerRow;
    }
}

void ACanvasLin4::DrawBitmapRle8U(const ABitmap &source, int nX, int nY) {
    // Each row is decoded into the shared scratch row and written from there, so the compressed
    // stream is consumed in order without this routine tracking it.
    ARle8Reader reader;
    reader.mSource = static_cast<const unsigned char *>(source.mPixels);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = source.mHasTransparentColor ?
                                   static_cast<int>(source.mTransparentColor) :
                                   kARleReaderNoTransparentValue;
    for (int nRow = nY; nRow < nY + source.mHeight; ++nRow) {
        (void)reader.UnpackRow(ACanvas::tempBuff); // The advanced destination is discarded.
        DrawBitmapRowLin8U(&source, ACanvas::tempBuff, nX, nRow);
    }
}

void ACanvasLin4::DrawBitmapRowLin8U(const ABitmap *pSource,
                                     const unsigned char *pRow,
                                     int nX,
                                     int nY) {
    // The bulk loop packs with a bitwise OR, and both per pixel paths store a logical OR in the
    // same position. Each of those stores writes 0 or 1 over the whole byte, a reading that
    // accounts for both destination meanings the binary shows.
    unsigned char *pDest = static_cast<unsigned char *>(mBitmap.mPixels) +
                           (nY * mBitmap.mBytesPerRow) +
                           ((nX + mBitmap.mOddNibbleStart) / kPixelsPerByte);
    int nCount = pSource->mWidth;
    if (((nX ^ mBitmap.mOddNibbleStart) & 1) != 0) {
        const unsigned char nIndex = *pRow++;
        if (pSource->mHasTransparentColor == 0 || nIndex != pSource->mTransparentColor) {
            *pDest = (*pDest & kNibbleMask) || nIndex; // Yes, a logical OR, as in the binary.
        }
        ++pDest;
        ++nX;
        --nCount;
    }
    if (pSource->mHasTransparentColor == 0 && nCount > 0) {
        int nPairs = nCount / kPixelsPerByte;
        while (nPairs-- != 0) {
            const unsigned char nLow = *pRow++;
            const unsigned char nHigh = *pRow++;
            *pDest++ = static_cast<unsigned char>(nLow | (nHigh << kNibbleShift));
        }
        // Yes, the binary advances by the exhausted counter, which is -1 here. nX moves back by two
        // and nCount grows by two, so the loop below reads and writes past the end of the row.
        nX += nPairs * kPixelsPerByte;
        nCount -= nPairs * kPixelsPerByte;
    }
    while (nCount-- != 0) {
        const unsigned char nIndex = *pRow++;
        if (pSource->mHasTransparentColor == 0 || nIndex != pSource->mTransparentColor) {
            // Yes, a logical OR in both arms, as in the binary.
            if (((nX ^ mBitmap.mOddNibbleStart) & 1) != 0) {
                *pDest = (*pDest & kNibbleMask) || nIndex;
            } else {
                *pDest = (*pDest & (kNibbleMask << kNibbleShift)) || nIndex;
            }
        }
        ++nX;
        if ((nX & 1) == 0) {
            ++pDest;
        }
    }
}
