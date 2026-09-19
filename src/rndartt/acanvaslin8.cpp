#include "rndartt/acanvaslin8.h"

#include <string.h>

namespace {

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
