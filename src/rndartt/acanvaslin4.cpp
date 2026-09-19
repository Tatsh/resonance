#include "rndartt/acanvaslin4.h"

#include <string.h>

#include "rndartt/arlereader.h"

namespace {

constexpr unsigned int kNibbleMask = 0xf;
constexpr int kNibbleShift = 4;
constexpr int kPixelsPerByte = 2;

} // namespace

// 0x006280d0
ACanvasLin4::ACanvasLin4(const ABitmap &bitmap) : ACanvas8(bitmap) {
    mColor = 0;
}

// 0x00628018
ACanvasLin4::~ACanvasLin4() {
}

// 0x00628108
void ACanvasLin4::PutPixelNoClip(int nX, int nY) {
    unsigned char *pByte = static_cast<unsigned char *>(mBitmap.mPixels) +
                           (nY * mBitmap.mBytesPerRow) + ((nX + mBitmap.mOddNibbleStart) / 2);
    if (((nX ^ mBitmap.mOddNibbleStart) & 1) != 0) {
        *pByte = static_cast<unsigned char>((*pByte & kNibbleMask) | (mColor << kNibbleShift));
    } else {
        *pByte = static_cast<unsigned char>((*pByte & (kNibbleMask << kNibbleShift)) |
                                            (mColor & kNibbleMask));
    }
}

// 0x00628180
void ACanvasLin4::PutPixelIndexedNoClip(int nX, int nY, int nIndex) {
    const unsigned int nValue = static_cast<unsigned int>(nIndex) & 0xff;
    unsigned char *pByte = static_cast<unsigned char *>(mBitmap.mPixels) +
                           (nY * mBitmap.mBytesPerRow) + ((nX + mBitmap.mOddNibbleStart) / 2);
    if (((nX ^ mBitmap.mOddNibbleStart) & 1) != 0) {
        *pByte = static_cast<unsigned char>((*pByte & kNibbleMask) | (nValue << kNibbleShift));
    } else {
        *pByte = static_cast<unsigned char>((*pByte & (kNibbleMask << kNibbleShift)) |
                                            (nValue & kNibbleMask));
    }
}

// 0x006281f0
int ACanvasLin4::GetPixelIndexedNoClip(int nX, int nY) {
    const unsigned char *pByte = static_cast<const unsigned char *>(mBitmap.mPixels) +
                                 (nY * mBitmap.mBytesPerRow) + ((nX + mBitmap.mOddNibbleStart) / 2);
    if (((nX ^ mBitmap.mOddNibbleStart) & 1) != 0) {
        return *pByte >> kNibbleShift;
    }
    return *pByte & kNibbleMask;
}

// 0x00627e88. Five conditions must all hold for the block copy: the source has no transparent
// colour, neither bitmap starts on an odd nibble, and the destination column and the source width
// are both even. Any one of them failing drops to unpacking each row into the shared scratch row
// and writing it back a pixel at a time.
void ACanvasLin4::Blit4NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    const bool bAligned = !source.mHasTransparentColor && mBitmap.mOddNibbleStart == 0 &&
                          source.mOddNibbleStart == 0 && (nX & 1) == 0 && (source.mWidth & 1) == 0;
    if (bAligned) {
        unsigned char *pDest = static_cast<unsigned char *>(mBitmap.mPixels) +
                               (nY * mBitmap.mBytesPerRow) +
                               ((nX + mBitmap.mOddNibbleStart) / kPixelsPerByte);
        for (int nRow = source.mHeight; nRow > 0; --nRow) {
            memcpy(pDest, pSourceRow, static_cast<unsigned int>(source.mWidth / kPixelsPerByte));
            pDest += mBitmap.mBytesPerRow;
            pSourceRow += source.mBytesPerRow;
        }
        return;
    }
    for (int nRow = 0; nRow < source.mHeight; ++nRow) {
        UnpackNibbleRow(pSourceRow, g_abCanvasRowScratch, source.mWidth, source.mOddNibbleStart);
        WriteIndexedRow(&source, g_abCanvasRowScratch, nX, nY + nRow);
        pSourceRow += source.mBytesPerRow;
    }
}

// 0x00628248. The destination row advances by two per source row, which covers every other row of
// the destination and consumes half the source height. The run length encoded sibling advances by
// one. Both behaviours match the binary.
void ACanvasLin4::Blit8NoClip(const ABitmap &source, int nX, int nY) {
    const unsigned char *pSourceRow = static_cast<const unsigned char *>(source.mPixels);
    for (int nRow = nY; nRow < nY + source.mHeight; nRow += 2) {
        WriteIndexedRow(&source, pSourceRow, nX, nRow);
        pSourceRow += source.mBytesPerRow;
    }
}

// 0x006282e0. Each row is decoded into the shared scratch row and written from there, so the
// compressed stream is consumed in order without this routine tracking it.
void ACanvasLin4::BlitRle8NoClip(const ABitmap &source, int nX, int nY) {
    ARleReader reader;
    reader.mSource = static_cast<const unsigned char *>(source.mPixels);
    reader.mWidth = source.mWidth;
    reader.mTransparentValue = source.mHasTransparentColor ?
                                   static_cast<int>(source.mTransparentColor) :
                                   kARleReaderNoTransparentValue;
    for (int nRow = nY; nRow < nY + source.mHeight; ++nRow) {
        (void)reader.DecodeRow(g_abCanvasRowScratch); // The advanced destination is discarded.
        WriteIndexedRow(&source, g_abCanvasRowScratch, nX, nRow);
    }
}
