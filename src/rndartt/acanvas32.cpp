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
// GetColorIndex() and GetPixelIndexedNoClip() form the same index before consulting the inverse
// lookup table of the palette.
inline unsigned int Rgb15Index(unsigned int nColor) {
    return ((nColor & kChannel5Mask) >> kChannel5Shift) |
           (((nColor >> kGreenShift) & kChannel5Mask) << 2) |
           (((nColor >> kBlueShift) & kChannel5Mask) << 7);
}

} // namespace

// 0x0062f790
ACanvas32::ACanvas32(const ABitmap &bitmap) : ACanvas(bitmap), mColor(0) {
}

// 0x0062f8c0
void ACanvas32::SetColorIndex(int nIndex) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = g_pDefaultPalette;
        if (pPalette == nullptr) {
            return;
        }
    }
    mColor = pPalette->mEntries[nIndex & kChannelMask];
}

// 0x0062f7c8
void ACanvas32::SetColor15(unsigned short nColor) {
    // Yes, the compiled body is a bare return. A 1555 pen colour has no effect here.
}

// 0x0062f7d0
void ACanvas32::SetColorRGB(const unsigned char *pRGB) {
    mColor = pRGB[0] | (pRGB[1] << kGreenShift) | (pRGB[2] << kBlueShift) |
             (kChannelMask << kAlphaShift);
}

// 0x0062f7f8
void ACanvas32::SetColor32(unsigned int nColor) {
    mColor = nColor;
}

// 0x0062f800
void ACanvas32::SetColorNative(unsigned int nColor) {
    mColor = nColor;
}

// 0x0062f668
int ACanvas32::GetColorIndex() {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = g_pDefaultPalette;
        if (pPalette == nullptr) {
            return 0;
        }
    }
    if (pPalette->mpRgb15ToIndex != nullptr) {
        return pPalette->mpRgb15ToIndex[Rgb15Index(mColor)] & kChannelMask;
    }
    return pPalette->FindNearestEntry((mColor & kACanvas32ChannelsMask) | kACanvas32AlphaOpaque,
                                      kPaletteFirstIndex,
                                      kPaletteLastIndex) &
           kChannelMask;
}

// 0x0062f808
unsigned short ACanvas32::GetColor15() {
    return static_cast<unsigned short>(Rgb15Index(mColor) | kAlpha15Bit);
}

// 0x0062f840
void ACanvas32::GetColorRGB(unsigned char *pRGB) {
    pRGB[0] = static_cast<unsigned char>(mColor & kChannelMask);
    pRGB[1] = static_cast<unsigned char>((mColor >> kGreenShift) & kChannelMask);
    pRGB[2] = static_cast<unsigned char>((mColor >> kBlueShift) & kChannelMask);
}

// 0x0062f860
unsigned int ACanvas32::GetColor32() {
    return mColor;
}

// 0x0062f868
unsigned int ACanvas32::GetColorNative() {
    return mColor;
}

// 0x0062f8f8
void ACanvas32::PutPixelIndexedNoClip(int nX, int nY, int nIndex) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = g_pDefaultPalette;
        if (pPalette == nullptr) {
            return;
        }
    }
    PutPixelNoClip(nX, nY, pPalette->mEntries[nIndex & kChannelMask]);
}

// 0x0062f950
void ACanvas32::PutPixel15NoClip(int nX, int nY, unsigned short nColor) {
    unsigned int nExpanded = ((nColor & kRed15Mask) << kChannel5Shift) |
                             ((nColor & kGreen15Mask) << 6) | ((nColor & kBlue15Mask) << 9);
    if ((nColor & kAlpha15Bit) != 0) {
        nExpanded |= kACanvas32AlphaOpaque;
    }
    PutPixelNoClip(nX, nY, nExpanded);
}

// 0x0062f9b8
void ACanvas32::PutPixelRGBNoClip(int nX, int nY, const unsigned char *pRGB) {
    PutPixelNoClip(nX,
                   nY,
                   pRGB[0] | (pRGB[1] << kGreenShift) | (pRGB[2] << kBlueShift) |
                       kACanvas32AlphaOpaque);
}

// 0x0062f870
void ACanvas32::PutPixelNativeNoClip(int nX, int nY, unsigned int nColor) {
    PutPixelNoClip(nX, nY, nColor);
}

// 0x0062fa08
int ACanvas32::GetPixelIndexedNoClip(int nX, int nY) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = g_pDefaultPalette;
        if (pPalette == nullptr) {
            return 0;
        }
    }
    const unsigned int nColor = GetPixelNoClip(nX, nY);
    if (pPalette->mpRgb15ToIndex != nullptr) {
        return pPalette->mpRgb15ToIndex[Rgb15Index(nColor)] & kChannelMask;
    }
    return pPalette->FindNearestEntry(nColor, kPaletteFirstIndex, kPaletteLastIndex) & kChannelMask;
}

// 0x0062faa0
unsigned short ACanvas32::GetPixel15NoClip(int nX, int nY) {
    const unsigned int nColor = GetPixelNoClip(nX, nY);
    return static_cast<unsigned short>(Rgb15Index(nColor) | ((nColor >> kBlueShift) & kAlpha15Bit));
}

// 0x0062faf8
void ACanvas32::GetPixelRGBNoClip(int nX, int nY, unsigned char *pRGB) {
    const unsigned int nColor = GetPixelNoClip(nX, nY);
    pRGB[0] = static_cast<unsigned char>(nColor & kChannelMask);
    pRGB[1] = static_cast<unsigned char>((nColor >> kGreenShift) & kChannelMask);
    pRGB[2] = static_cast<unsigned char>((nColor >> kBlueShift) & kChannelMask);
}

// 0x0062f898
unsigned int ACanvas32::GetPixelNativeNoClip(int nX, int nY) {
    return GetPixelNoClip(nX, nY);
}
