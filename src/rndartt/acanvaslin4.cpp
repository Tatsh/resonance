#include "rndartt/acanvaslin4.h"

namespace {

constexpr unsigned int kNibbleMask = 0xf;
constexpr int kNibbleShift = 4;

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
