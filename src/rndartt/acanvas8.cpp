#include "rndartt/acanvas8.h"

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
constexpr unsigned int kRgb32Mask = 0x00ffffff;

constexpr int kPaletteFirstIndex = 0;
constexpr int kPaletteLastIndex = 255;

// Expand a 1555 colour to 8888, exactly as ACanvas15 does.
inline unsigned int Rgb8888From1555(unsigned int nColor) {
    unsigned int nResult = ((nColor & kRed15Mask) << kChannel5Shift) |
                           ((nColor & kGreen15Mask) << 6) | ((nColor & kBlue15Mask) << 9);
    if ((nColor & kAlpha15Bit) != 0) {
        nResult |= kAlpha32Opaque;
    }
    return nResult;
}

// Pack an 8888 colour into a 1555 lookup key. This DROPS the alpha bit, where the same conversion
// in ACanvas15 carries it: the result here only ever indexes the palette's reverse table, for which
// alpha is meaningless.
inline unsigned int Rgb15KeyFrom8888(unsigned int nColor) {
    return ((nColor & kChannel5Mask) >> kChannel5Shift) |
           (((nColor >> kGreenShift) & kChannel5Mask) << 2) |
           (((nColor >> kBlueShift) & kChannel5Mask) << 7);
}

// Pack three channel bytes into a 1555 lookup key, in the same field order.
inline unsigned int Rgb15KeyFromChannels(const unsigned char *pRGB) {
    return (static_cast<unsigned int>(pRGB[0]) >> kChannel5Shift) |
           ((pRGB[1] & kChannel5Mask) << 2) | ((pRGB[2] & kChannel5Mask) << 7);
}

// Pack three channel bytes into 8888 with alpha opaque.
inline unsigned int Rgb8888FromChannels(const unsigned char *pRGB) {
    return pRGB[0] | (static_cast<unsigned int>(pRGB[1]) << kGreenShift) |
           (static_cast<unsigned int>(pRGB[2]) << kBlueShift) | kAlpha32Opaque;
}

// Every colour and pixel slot opens with this resolution, fifteen times in the original. It is
// written as one inline helper here rather than repeated, which is the de-inlining the
// reconstruction rules sanction for a repeated block; ACanvas15 repeats it instead.
inline APalette *ResolvePalette(APalette *pOwn) {
    return pOwn != nullptr ? pOwn : ACanvas::palDefault;
}

} // namespace

ACanvas8::ACanvas8(const ABitmap &bitmap) : ACanvas(bitmap), mColor(0) {
}

ACanvas8::~ACanvas8() {
}

void ACanvas8::SetColor8(int nIndex) {
    mColor = static_cast<unsigned char>(nIndex);
}

void ACanvas8::SetColorNative(unsigned int nColor) {
    mColor = static_cast<unsigned char>(nColor);
}

int ACanvas8::GetColor8() {
    return mColor;
}

unsigned int ACanvas8::GetColorNative() {
    return mColor;
}

void ACanvas8::SetColor15(unsigned short nColor) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    if (pPalette->mpRgb15ToIndex != nullptr) {
        mColor = pPalette->mpRgb15ToIndex[nColor & kColor15Mask];
        return;
    }
    mColor = static_cast<unsigned char>(
        pPalette->FindClosest(Rgb8888From1555(nColor), kPaletteFirstIndex, kPaletteLastIndex));
}

void ACanvas8::SetColor24(const unsigned char *pRGB) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    if (pPalette->mpRgb15ToIndex != nullptr) {
        mColor = pPalette->mpRgb15ToIndex[Rgb15KeyFromChannels(pRGB)];
        return;
    }
    mColor = static_cast<unsigned char>(
        pPalette->FindClosest(Rgb8888FromChannels(pRGB), kPaletteFirstIndex, kPaletteLastIndex));
}

void ACanvas8::SetColor32(unsigned int nColor) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    if (pPalette->mpRgb15ToIndex != nullptr) {
        mColor = pPalette->mpRgb15ToIndex[Rgb15KeyFrom8888(nColor)];
        return;
    }
    mColor = static_cast<unsigned char>(
        pPalette->FindClosest(nColor, kPaletteFirstIndex, kPaletteLastIndex));
}

unsigned short ACanvas8::GetColor15() {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return 0;
    }
    return APackRgb1555From8888(pPalette->mEntries[mColor]);
}

void ACanvas8::GetColor24(unsigned char *pRGB) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    const unsigned int nColor = pPalette->mEntries[mColor];
    pRGB[2] = static_cast<unsigned char>(nColor >> kBlueShift);
    pRGB[1] = static_cast<unsigned char>(nColor >> kGreenShift);
    pRGB[0] = static_cast<unsigned char>(nColor);
}

unsigned int ACanvas8::GetColor32() {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return 0;
    }
    return pPalette->mEntries[mColor];
}

void ACanvas8::SetAlphaValues(unsigned int nColorKey) {
    // The palette pointer and the entry count are re-read on every iteration rather than cached.
    if (mBitmap.mPalette == nullptr) {
        return;
    }
    for (int nIndex = 0; nIndex < mBitmap.mPalette->mEnd; ++nIndex) {
        const unsigned int nEntry = mBitmap.mPalette->mEntries[nIndex];
        mBitmap.mPalette->mEntries[nIndex] = static_cast<unsigned int>(nIndex) == nColorKey ?
                                                 (nEntry & kRgb32Mask) :
                                                 (nEntry | kAlpha32Opaque);
    }
}

void ACanvas8::DrawPixelNativeU(int nX, int nY, unsigned int nColor) {
    DrawPixel8U(nX, nY, static_cast<int>(nColor & kChannelMask));
}

void ACanvas8::DrawPixel15U(int nX, int nY, unsigned short nColor) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    int nIndex;
    if (pPalette->mpRgb15ToIndex != nullptr) {
        nIndex = pPalette->mpRgb15ToIndex[nColor & kColor15Mask];
    } else {
        nIndex =
            pPalette->FindClosest(Rgb8888From1555(nColor), kPaletteFirstIndex, kPaletteLastIndex);
    }
    DrawPixel8U(nX, nY, nIndex & static_cast<int>(kChannelMask));
}

void ACanvas8::DrawPixel24U(int nX, int nY, const unsigned char *pRGB) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    int nIndex;
    if (pPalette->mpRgb15ToIndex != nullptr) {
        nIndex = pPalette->mpRgb15ToIndex[Rgb15KeyFromChannels(pRGB)];
    } else {
        nIndex =
            pPalette->FindClosest(Rgb8888FromChannels(pRGB), kPaletteFirstIndex, kPaletteLastIndex);
    }
    DrawPixel8U(nX, nY, nIndex & static_cast<int>(kChannelMask));
}

void ACanvas8::DrawPixel32U(int nX, int nY, unsigned int nColor) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    int nIndex;
    if (pPalette->mpRgb15ToIndex != nullptr) {
        nIndex = pPalette->mpRgb15ToIndex[Rgb15KeyFrom8888(nColor)];
    } else {
        nIndex = pPalette->FindClosest(nColor, kPaletteFirstIndex, kPaletteLastIndex);
    }
    DrawPixel8U(nX, nY, nIndex & static_cast<int>(kChannelMask));
}

unsigned int ACanvas8::GetPixelNativeU(int nX, int nY) {
    return static_cast<unsigned int>(GetPixel8U(nX, nY));
}

unsigned short ACanvas8::GetPixel15U(int nX, int nY) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return 0;
    }
    return APackRgb1555From8888(pPalette->mEntries[GetPixel8U(nX, nY)]);
}

void ACanvas8::GetPixel24U(int nX, int nY, unsigned char *pRGB) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    const unsigned int nColor = pPalette->mEntries[GetPixel8U(nX, nY)];
    pRGB[2] = static_cast<unsigned char>(nColor >> kBlueShift);
    pRGB[1] = static_cast<unsigned char>(nColor >> kGreenShift);
    pRGB[0] = static_cast<unsigned char>(nColor);
}

unsigned int ACanvas8::GetPixel32U(int nX, int nY) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return 0;
    }
    return pPalette->mEntries[GetPixel8U(nX, nY)];
}
