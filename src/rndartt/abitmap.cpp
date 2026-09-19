#include "rndartt/abitmap.h"

#include "os/mem.h"
#include "rndartt/apalette.h"

namespace {

// 0x0082b1c0, the only source path the shipped image retains for this file.
const char *const kAllocTag = "abitmap.h";
constexpr int kAllocLine = 0x47;

} // namespace

// 0x00725cc0
const unsigned char g_abBitmapBytesPerPixel[kABitmapFormatCount] = {0, 1, 2, 3, 4, 1};

// 0x00725cc8
const unsigned char g_abBitmapBitsPerPixel[kABitmapFormatCount] = {4, 8, 16, 24, 32, 8};

// 0x005593a0
ABitmap::ABitmap(void *pPixels,
                 int nFormat,
                 bool bHasTransparentColor,
                 int nWidth,
                 int nHeight,
                 int nBytesPerRow) {
    mHeight = static_cast<short>(nHeight);
    mFormat = nFormat;
    mHasTransparentColor = bHasTransparentColor;
    mOddNibbleStart = 0;
    mWidth = static_cast<short>(nWidth);
    if (nBytesPerRow != 0) {
        mBytesPerRow = static_cast<short>(nBytesPerRow);
    } else if (mFormat == kABitmapFormatLinear4) {
        mBytesPerRow = static_cast<short>((mWidth + 2) / 2);
    } else {
        mBytesPerRow = static_cast<short>(mWidth * g_abBitmapBytesPerPixel[mFormat]);
    }
    mTransparentColor = 0;
    mByteCount = mBytesPerRow * mHeight;
    mPixels = pPixels;
    if (pPixels == nullptr) {
        mPixels = MemAllocTagged(mByteCount, kAllocTag, kAllocLine);
        mOwnsPixels = 1;
    } else {
        mOwnsPixels = 0;
    }
    mPalette = nullptr;
}

// 0x005eb290
void ABitmap::SetPaletteEntries(const unsigned int *pEntries, int nFirst, int nCount) {
    if (mPalette == nullptr) {
        APalette *pPalette =
            static_cast<APalette *>(AllocateTaggedMemory(sizeof(APalette), "APalette"));
        // Yes, the binary writes through the block and stores it before testing it against null.
        pPalette->mpRgb15ToIndex = nullptr;
        pPalette->mEnd = 0;
        mPalette = pPalette;
        if (pPalette == nullptr) {
            return;
        }
    }
    mPalette->SetEntries(pEntries, nFirst, nCount);
}
