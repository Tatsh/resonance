#include "rndartt/acanvas32.h"

#include "rndartt/apalette.h"

namespace {

constexpr unsigned int kChannelMask = 0xff;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;
constexpr int kAlphaShift = 24;

// 1555 field positions and the five bit channel mask inside an eight bit channel.
constexpr unsigned int kRed15Mask = 0x001f;
constexpr unsigned int kGreen15Mask = 0x03e0;
constexpr unsigned int kBlue15Mask = 0x7c00;
constexpr unsigned int kAlpha15Bit = 0x8000;
constexpr unsigned int kChannel5Mask = 0xf8;
constexpr int kChannel5Shift = 3;

constexpr int kPaletteFirstIndex = 0;
constexpr int kPaletteLastIndex = 255;

// Pack the three colour channels of an 8888 word into a 1555 index, alpha excluded. Both
// GetColor8() and GetPixel8U() form the same index before consulting the inverse
// lookup table of the palette.
inline unsigned int Rgb15Index(unsigned int nColor) {
    return ((nColor & kChannel5Mask) >> kChannel5Shift) |
           (((nColor >> kGreenShift) & kChannel5Mask) << 2) |
           (((nColor >> kBlueShift) & kChannel5Mask) << 7);
}

} // namespace

ACanvas32::ACanvas32(const ABitmap &bitmap) : ACanvas(bitmap), mColor(0) {
}

void ACanvas32::SetColor8(int nIndex) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = ACanvas::palDefault;
        if (pPalette == nullptr) {
            return;
        }
    }
    mColor = pPalette->mEntries[nIndex & kChannelMask];
}

void ACanvas32::SetColor15([[maybe_unused]] unsigned short nColor) {
    // Yes, the compiled body is a bare return. A 1555 pen colour has no effect here.
}

void ACanvas32::SetColor24(const unsigned char *pRGB) {
    mColor = pRGB[0] | (pRGB[1] << kGreenShift) | (pRGB[2] << kBlueShift) |
             (kChannelMask << kAlphaShift);
}

void ACanvas32::SetColor32(unsigned int nColor) {
    mColor = nColor;
}

void ACanvas32::SetColorNative(unsigned int nColor) {
    mColor = nColor;
}

int ACanvas32::GetColor8() {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = ACanvas::palDefault;
        if (pPalette == nullptr) {
            return 0;
        }
    }
    if (pPalette->mpRgb15ToIndex != nullptr) {
        return pPalette->mpRgb15ToIndex[Rgb15Index(mColor)] & kChannelMask;
    }
    return pPalette->FindClosest((mColor & kACanvas32ChannelsMask) | kACanvas32AlphaOpaque,
                                 kPaletteFirstIndex,
                                 kPaletteLastIndex) &
           kChannelMask;
}

unsigned short ACanvas32::GetColor15() {
    return static_cast<unsigned short>(Rgb15Index(mColor) | kAlpha15Bit);
}

void ACanvas32::GetColor24(unsigned char *pRGB) {
    pRGB[0] = static_cast<unsigned char>(mColor & kChannelMask);
    pRGB[1] = static_cast<unsigned char>((mColor >> kGreenShift) & kChannelMask);
    pRGB[2] = static_cast<unsigned char>((mColor >> kBlueShift) & kChannelMask);
}

unsigned int ACanvas32::GetColor32() {
    return mColor;
}

unsigned int ACanvas32::GetColorNative() {
    return mColor;
}

void ACanvas32::DrawPixel8U(int nX, int nY, int nIndex) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = ACanvas::palDefault;
        if (pPalette == nullptr) {
            return;
        }
    }
    DrawPixel32U(nX, nY, pPalette->mEntries[nIndex & kChannelMask]);
}

void ACanvas32::DrawPixel15U(int nX, int nY, unsigned short nColor) {
    unsigned int nExpanded = ((nColor & kRed15Mask) << kChannel5Shift) |
                             ((nColor & kGreen15Mask) << 6) | ((nColor & kBlue15Mask) << 9);
    if ((nColor & kAlpha15Bit) != 0) {
        nExpanded |= kACanvas32AlphaOpaque;
    }
    DrawPixel32U(nX, nY, nExpanded);
}

void ACanvas32::DrawPixel24U(int nX, int nY, const unsigned char *pRGB) {
    DrawPixel32U(nX,
                 nY,
                 pRGB[0] | (pRGB[1] << kGreenShift) | (pRGB[2] << kBlueShift) |
                     kACanvas32AlphaOpaque);
}

void ACanvas32::DrawPixelNativeU(int nX, int nY, unsigned int nColor) {
    DrawPixel32U(nX, nY, nColor);
}

int ACanvas32::GetPixel8U(int nX, int nY) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = ACanvas::palDefault;
        if (pPalette == nullptr) {
            return 0;
        }
    }
    const unsigned int nColor = GetPixel32U(nX, nY);
    if (pPalette->mpRgb15ToIndex != nullptr) {
        return pPalette->mpRgb15ToIndex[Rgb15Index(nColor)] & kChannelMask;
    }
    return pPalette->FindClosest(nColor, kPaletteFirstIndex, kPaletteLastIndex) & kChannelMask;
}

unsigned short ACanvas32::GetPixel15U(int nX, int nY) {
    const unsigned int nColor = GetPixel32U(nX, nY);
    return static_cast<unsigned short>(Rgb15Index(nColor) | ((nColor >> kBlueShift) & kAlpha15Bit));
}

void ACanvas32::GetPixel24U(int nX, int nY, unsigned char *pRGB) {
    const unsigned int nColor = GetPixel32U(nX, nY);
    pRGB[0] = static_cast<unsigned char>(nColor & kChannelMask);
    pRGB[1] = static_cast<unsigned char>((nColor >> kGreenShift) & kChannelMask);
    pRGB[2] = static_cast<unsigned char>((nColor >> kBlueShift) & kChannelMask);
}

unsigned int ACanvas32::GetPixelNativeU(int nX, int nY) {
    return GetPixel32U(nX, nY);
}
