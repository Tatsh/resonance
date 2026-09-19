#include "rndartt/acanvaslin15.h"

namespace {

constexpr unsigned int kColor15Mask = 0x7fff;
constexpr unsigned int kAlpha15Bit = 0x8000;
constexpr int kBytesPerPixel = 2;

// Two bytes are one pixel. The row offset is a byte count and the column is a pixel index, which
// is how the binary computes it: the row is added as bytes and the column shifted left by one.
inline unsigned short *RowAt(void *pPixels, int nBytesPerRow, int nY) {
    void *pRow = static_cast<unsigned char *>(pPixels) + (nY * nBytesPerRow);
    return static_cast<unsigned short *>(pRow);
}

inline unsigned short *PixelAt(void *pPixels, int nBytesPerRow, int nX, int nY) {
    return RowAt(pPixels, nBytesPerRow, nY) + nX;
}

} // namespace

// 0x00619008
ACanvasLin15::ACanvasLin15(const ABitmap &bitmap) : ACanvas15(bitmap) {
    mColor = 0;
}

// 0x00618f68
ACanvasLin15::~ACanvasLin15() {
}

// 0x00619040. The width, the height, and the row pitch are all re-read inside the loops rather
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

// 0x00619128. A halfword store per pixel rather than a memset, and the colour is re-read on every
// iteration.
void ACanvasLin15::FillRowNoClip(int nY, int nLeft, int nRight) {
    unsigned short *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nLeft, nY);
    for (int nCount = nRight - nLeft; nCount != 0; --nCount) {
        *pPixel = mColor;
        ++pPixel;
    }
}

// 0x00619170. The colour and the row pitch are both re-read on every iteration, which recomputing
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
