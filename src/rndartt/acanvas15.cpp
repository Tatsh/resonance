#include "rndartt/acanvas15.h"

#include "rndartt/apalette.h"

namespace {

constexpr unsigned int kChannelMask = 0xff;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;

// 1555 field positions, and the five bit channel mask inside an eight bit channel.
constexpr unsigned int kRed15Mask = 0x001f;
constexpr unsigned int kGreen15Mask = 0x03e0;
constexpr unsigned int kBlue15Mask = 0x7c00;
constexpr unsigned int kAlpha15Bit = 0x8000;
constexpr unsigned int kColor15Mask = 0x7fff;
constexpr unsigned int kChannel5Mask = 0xf8;
constexpr int kChannel5Shift = 3;

constexpr unsigned int kAlpha32Opaque = 0xff000000;

constexpr int kPaletteFirstIndex = 0;
constexpr int kPaletteLastIndex = 255;

// Expand a 1555 colour to 8888. Alpha is all ones when the 1555 alpha bit is set and zero
// otherwise, and the low three bits of every channel expand to zero rather than being replicated.
// GetColor32(), GetPixelNoClip(), and both index lookups form the same value.
inline unsigned int Rgb8888From1555(unsigned int nColor) {
    unsigned int nResult = ((nColor & kRed15Mask) << kChannel5Shift) |
                           ((nColor & kGreen15Mask) << 6) | ((nColor & kBlue15Mask) << 9);
    if ((nColor & kAlpha15Bit) != 0) {
        nResult |= kAlpha32Opaque;
    }
    return nResult;
}

// Pack an 8888 colour into 1555, alpha bit from bit 31. SetColor32() and PutPixelNoClip() both
// write this sequence out rather than reaching APackRgb1555From8888(), which is the same packing
// out of line.
inline unsigned short Rgb1555From8888(unsigned int nColor) {
    return static_cast<unsigned short>(((nColor & kChannel5Mask) >> kChannel5Shift) |
                                       (((nColor >> kGreenShift) & kChannel5Mask) << 2) |
                                       (((nColor >> kBlueShift) & kChannel5Mask) << 7) |
                                       ((nColor >> kBlueShift) & kAlpha15Bit));
}

// Pack three channel bytes into 1555 with the alpha bit set. SetColorRGB() and
// PutPixelRGBNoClip() form the same value.
inline unsigned short Rgb1555FromChannels(const unsigned char *pRGB) {
    return static_cast<unsigned short>((pRGB[0] >> kChannel5Shift) |
                                       ((pRGB[1] & kChannel5Mask) << 2) |
                                       ((pRGB[2] & kChannel5Mask) << 7) | kAlpha15Bit);
}

} // namespace

// NTSC-U/C: 0x0062fbc0, PAL: 0x00670750
ACanvas15::ACanvas15(const ABitmap &bitmap) : ACanvas(bitmap), mColor(0) {
}

// NTSC-U/C: 0x0062fd48, PAL: 0x006708d8
void ACanvas15::SetColorIndex(int nIndex) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = g_pDefaultPalette;
        if (pPalette == nullptr) {
            return;
        }
    }
    mColor = APackRgb1555From8888(pPalette->mEntries[nIndex & kChannelMask]);
}

// NTSC-U/C: 0x0062fbf8, PAL: 0x00670788
void ACanvas15::SetColor15(unsigned short nColor) {
    mColor = nColor;
}

// NTSC-U/C: 0x0062fc00, PAL: 0x00670790
void ACanvas15::SetColorRGB(const unsigned char *pRGB) {
    mColor = Rgb1555FromChannels(pRGB);
}

// NTSC-U/C: 0x0062fc38, PAL: 0x006707c8
void ACanvas15::SetColor32(unsigned int nColor) {
    mColor = Rgb1555From8888(nColor);
}

// NTSC-U/C: 0x0062fc70, PAL: 0x00670800
void ACanvas15::SetColorNative(unsigned int nColor) {
    mColor = static_cast<unsigned short>(nColor);
}

// NTSC-U/C: 0x0062fd98, PAL: 0x00670928
int ACanvas15::GetColorIndex() {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = g_pDefaultPalette;
        if (pPalette == nullptr) {
            return 0;
        }
    }
    if (pPalette->mpRgb15ToIndex != nullptr) {
        return pPalette->mpRgb15ToIndex[mColor & kColor15Mask] & kChannelMask;
    }
    return pPalette->FindNearestEntry(
               Rgb8888From1555(mColor), kPaletteFirstIndex, kPaletteLastIndex) &
           kChannelMask;
}

// NTSC-U/C: 0x0062fc78, PAL: 0x00670808
unsigned short ACanvas15::GetColor15() {
    return mColor;
}

// NTSC-U/C: 0x0062fc80, PAL: 0x00670810
void ACanvas15::GetColorRGB(unsigned char *pRGB) {
    pRGB[0] = static_cast<unsigned char>(mColor << kChannel5Shift);
    pRGB[1] = static_cast<unsigned char>((mColor >> 2) & kChannel5Mask);
    pRGB[2] = static_cast<unsigned char>((mColor >> 7) & kChannel5Mask);
}

// NTSC-U/C: 0x0062fca8, PAL: 0x00670838
unsigned int ACanvas15::GetColor32() {
    return Rgb8888From1555(mColor);
}

// NTSC-U/C: 0x0062fce8, PAL: 0x00670878
unsigned int ACanvas15::GetColorNative() {
    return mColor;
}

// NTSC-U/C: 0x0062fe28, PAL: 0x006709b8
void ACanvas15::PutPixelIndexedNoClip(int nX, int nY, int nIndex) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = g_pDefaultPalette;
        if (pPalette == nullptr) {
            return;
        }
    }
    PutPixel15NoClip(nX, nY, APackRgb1555From8888(pPalette->mEntries[nIndex & kChannelMask]));
}

// NTSC-U/C: 0x0062fec0, PAL: 0x00670a50
void ACanvas15::PutPixelRGBNoClip(int nX, int nY, const unsigned char *pRGB) {
    PutPixel15NoClip(nX, nY, Rgb1555FromChannels(pRGB));
}

// NTSC-U/C: 0x0062ff18, PAL: 0x00670aa8
void ACanvas15::PutPixelNoClip(int nX, int nY, unsigned int nColor) {
    PutPixel15NoClip(nX, nY, Rgb1555From8888(nColor));
}

// NTSC-U/C: 0x0062fcf0, PAL: 0x00670880
void ACanvas15::PutPixelNativeNoClip(int nX, int nY, unsigned int nColor) {
    PutPixel15NoClip(nX, nY, static_cast<unsigned short>(nColor));
}

// NTSC-U/C: 0x0062ff70, PAL: 0x00670b00
int ACanvas15::GetPixelIndexedNoClip(int nX, int nY) {
    APalette *pPalette = mBitmap.mPalette;
    if (pPalette == nullptr) {
        pPalette = g_pDefaultPalette;
        if (pPalette == nullptr) {
            return 0;
        }
    }
    const unsigned int nColor = GetPixel15NoClip(nX, nY);
    if (pPalette->mpRgb15ToIndex != nullptr) {
        return pPalette->mpRgb15ToIndex[nColor & kColor15Mask] & kChannelMask;
    }
    return pPalette->FindNearestEntry(
               Rgb8888From1555(nColor), kPaletteFirstIndex, kPaletteLastIndex) &
           kChannelMask;
}

// NTSC-U/C: 0x00630020, PAL: 0x00670bb0
void ACanvas15::GetPixelRGBNoClip(int nX, int nY, unsigned char *pRGB) {
    const unsigned int nColor = GetPixel15NoClip(nX, nY);
    pRGB[0] = static_cast<unsigned char>(nColor << kChannel5Shift);
    pRGB[1] = static_cast<unsigned char>((nColor >> 2) & kChannel5Mask);
    pRGB[2] = static_cast<unsigned char>((nColor >> 7) & kChannel5Mask);
}

// NTSC-U/C: 0x00630078, PAL: 0x00670c08
unsigned int ACanvas15::GetPixelNoClip(int nX, int nY) {
    return Rgb8888From1555(GetPixel15NoClip(nX, nY));
}

// NTSC-U/C: 0x0062fd20, PAL: 0x006708b0
unsigned int ACanvas15::GetPixelNativeNoClip(int nX, int nY) {
    return GetPixel15NoClip(nX, nY);
}
