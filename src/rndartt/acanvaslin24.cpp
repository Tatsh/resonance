#include "rndartt/acanvaslin24.h"

namespace {

constexpr int kBytesPerPixel = 3;

// Three bytes are one pixel. The binary forms the column offset as the index doubled plus itself
// rather than with a multiply.
inline unsigned char *PixelAt(void *pPixels, int nBytesPerRow, int nX, int nY) {
    void *pPixel = static_cast<unsigned char *>(pPixels) + (nY * nBytesPerRow);
    return static_cast<unsigned char *>(pPixel) + (nX * kBytesPerPixel);
}

} // namespace

// 0x00618470
ACanvasLin24::ACanvasLin24(const ABitmap &bitmap) : ACanvas24(bitmap) {
    mColorNative = 0;
}

// 0x006183d0
ACanvasLin24::~ACanvasLin24() {
}

// 0x006184a8. Empty in the binary, and the slot exists only because ACanvas24 declares it pure.
void ACanvasLin24::BuildAlphaFromColorKey(unsigned int nColorKey) {
    (void)nColorKey;
}

// 0x006184b0
void ACanvasLin24::PutPixelNoClip(int nX, int nY) {
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    pPixel[0] = mColorChannels[0];
    pPixel[1] = mColorChannels[1];
    pPixel[2] = mColorChannels[2];
}

// 0x006184e8
void ACanvasLin24::PutPixelRGBNoClip(int nX, int nY, const unsigned char *pRGB) {
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    pPixel[0] = pRGB[0];
    pPixel[1] = pRGB[1];
    pPixel[2] = pRGB[2];
}

// 0x00618528
void ACanvasLin24::GetPixelRGBNoClip(int nX, int nY, unsigned char *pRGB) {
    const unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nY);
    pRGB[0] = pPixel[0];
    pRGB[1] = pPixel[1];
    pRGB[2] = pPixel[2];
}

// 0x00618568. The three colour bytes are re-read from the object on every iteration rather than
// hoisted, and the destination advances one byte at a time rather than three at once.
void ACanvasLin24::FillRowNoClip(int nY, int nLeft, int nRight) {
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

// 0x006185d0. The row pitch is re-read from the bitmap on every iteration.
void ACanvasLin24::FillColumnNoClip(int nX, int nTop, int nBottom) {
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, nX, nTop);
    for (int nCount = nBottom - nTop; nCount != 0; --nCount) {
        pPixel[0] = mColorChannels[0];
        pPixel[1] = mColorChannels[1];
        pPixel[2] = mColorChannels[2];
        pPixel += mBitmap.mBytesPerRow;
    }
}

// 0x00618630
void ACanvasLin24::FillRectNoClip(ARect rect) {
    const int nColumns = rect.mRight - rect.mLeft;
    const int nRowAdvance = mBitmap.mBytesPerRow - (nColumns * kBytesPerPixel);
    unsigned char *pPixel = PixelAt(mBitmap.mPixels, mBitmap.mBytesPerRow, rect.mLeft, rect.mTop);
    for (int nRows = rect.mBottom - rect.mTop; nRows > 0; --nRows) {
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
