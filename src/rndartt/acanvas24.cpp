#include "rndartt/acanvas24.h"

#include "rndartt/apalette.h"

namespace {

constexpr unsigned int kChannelMask = 0xff;
constexpr int kGreenShift = 8;
constexpr int kBlueShift = 16;

constexpr unsigned int kChannel5Mask = 0xf8;
constexpr int kChannel5Shift = 3;
constexpr unsigned int kColor15Mask = 0xffff;
constexpr unsigned int kAlpha15Bit = 0x8000;

constexpr unsigned int kAlpha32Opaque = 0xff000000;

constexpr int kPaletteFirstIndex = 0;
constexpr int kPaletteLastIndex = 255;

constexpr int kRedChannel = 0;
constexpr int kGreenChannel = 1;
constexpr int kBlueChannel = 2;
constexpr int kChannelCount = 3;

// Every routine that needs a palette resolves it this way, falling back to the engine-wide one.
inline APalette *ResolvePalette(APalette *pOwn) {
    return pOwn != nullptr ? pOwn : g_pDefaultPalette;
}

// Expand a 1555 colour into three channel bytes. The low three bits of each channel expand to zero
// rather than being replicated.
inline void ChannelsFromColor15(unsigned int nColor, unsigned char *pRGB) {
    pRGB[kRedChannel] = static_cast<unsigned char>(nColor << kChannel5Shift);
    pRGB[kGreenChannel] = static_cast<unsigned char>((nColor >> 2) & kChannel5Mask);
    pRGB[kBlueChannel] = static_cast<unsigned char>((nColor >> 7) & kChannel5Mask);
}

// Split an 8888 colour into three channel bytes, discarding its alpha.
inline void ChannelsFrom8888(unsigned int nColor, unsigned char *pRGB) {
    pRGB[kRedChannel] = static_cast<unsigned char>(nColor);
    pRGB[kGreenChannel] = static_cast<unsigned char>(nColor >> kGreenShift);
    pRGB[kBlueChannel] = static_cast<unsigned char>(nColor >> kBlueShift);
}

// Pack three channel bytes into 1555 with the alpha bit always set.
inline unsigned short Color15FromChannels(const unsigned char *pRGB) {
    return static_cast<unsigned short>((pRGB[kRedChannel] >> kChannel5Shift) |
                                       ((pRGB[kGreenChannel] & kChannel5Mask) << 2) |
                                       ((pRGB[kBlueChannel] & kChannel5Mask) << 7) | kAlpha15Bit);
}

// Pack three channel bytes into the 1555 key the palette's reverse table is indexed by. The alpha
// bit is absent here, where Color15FromChannels() sets it, because a lookup key has no use for it.
inline unsigned int Rgb15KeyFromChannels(const unsigned char *pRGB) {
    return (static_cast<unsigned int>(pRGB[kRedChannel]) >> kChannel5Shift) |
           ((pRGB[kGreenChannel] & kChannel5Mask) << 2) |
           ((pRGB[kBlueChannel] & kChannel5Mask) << 7);
}

// Pack three channel bytes into 8888 with alpha opaque.
inline unsigned int Rgb8888FromChannels(const unsigned char *pRGB) {
    return pRGB[kRedChannel] | (static_cast<unsigned int>(pRGB[kGreenChannel]) << kGreenShift) |
           (static_cast<unsigned int>(pRGB[kBlueChannel]) << kBlueShift) | kAlpha32Opaque;
}

// Find the palette index nearest a channel triple, by reverse table where one exists and by search
// otherwise. GetColorIndex() and GetPixelIndexedNoClip() both perform this sequence.
inline int IndexForChannels(APalette *pPalette, const unsigned char *pRGB) {
    if (pPalette->mpRgb15ToIndex != nullptr) {
        return pPalette->mpRgb15ToIndex[Rgb15KeyFromChannels(pRGB)] &
               static_cast<int>(kChannelMask);
    }
    return pPalette->FindNearestEntry(
               Rgb8888FromChannels(pRGB), kPaletteFirstIndex, kPaletteLastIndex) &
           static_cast<int>(kChannelMask);
}

} // namespace

// 0x006302f8
ACanvas24::ACanvas24(const ABitmap &bitmap) : ACanvas(bitmap), mColorNative(0) {
}

// 0x00630278
ACanvas24::~ACanvas24() {
}

// 0x00630448
void ACanvas24::SetColorIndex(int nIndex) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    ChannelsFrom8888(pPalette->mEntries[nIndex & static_cast<int>(kChannelMask)], mColorChannels);
}

// 0x00630330
void ACanvas24::SetColor15(unsigned short nColor) {
    ChannelsFromColor15(nColor & kColor15Mask, mColorChannels);
}

// 0x00630360
void ACanvas24::SetColorRGB(const unsigned char *pRGB) {
    mColorChannels[kRedChannel] = pRGB[kRedChannel];
    mColorChannels[kGreenChannel] = pRGB[kGreenChannel];
    mColorChannels[kBlueChannel] = pRGB[kBlueChannel];
}

// 0x00630380. Byte for byte the same store as SetColorNative(), in a separate slot: for this
// format an 8888 colour and the native word are the same thing.
void ACanvas24::SetColor32(unsigned int nColor) {
    mColorNative = nColor;
}

// 0x00630388
void ACanvas24::SetColorNative(unsigned int nColor) {
    mColorNative = nColor;
}

// 0x00630108
int ACanvas24::GetColorIndex() {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return 0;
    }
    return IndexForChannels(pPalette, mColorChannels);
}

// 0x00630390
unsigned short ACanvas24::GetColor15() {
    return Color15FromChannels(mColorChannels);
}

// 0x006303c8
void ACanvas24::GetColorRGB(unsigned char *pRGB) {
    pRGB[kRedChannel] = mColorChannels[kRedChannel];
    pRGB[kGreenChannel] = mColorChannels[kGreenChannel];
    pRGB[kBlueChannel] = mColorChannels[kBlueChannel];
}

// 0x006303e8. Byte for byte the same load as GetColorNative(), in a separate slot.
unsigned int ACanvas24::GetColor32() {
    return mColorNative;
}

// 0x006303f0
unsigned int ACanvas24::GetColorNative() {
    return mColorNative;
}

// 0x00630498
void ACanvas24::PutPixelIndexedNoClip(int nX, int nY, int nIndex) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return;
    }
    unsigned char rgb[kChannelCount];
    ChannelsFrom8888(pPalette->mEntries[nIndex & static_cast<int>(kChannelMask)], rgb);
    PutPixelRGBNoClip(nX, nY, rgb);
}

// 0x00630508
void ACanvas24::PutPixel15NoClip(int nX, int nY, unsigned short nColor) {
    unsigned char rgb[kChannelCount];
    ChannelsFromColor15(nColor & kColor15Mask, rgb);
    PutPixelRGBNoClip(nX, nY, rgb);
}

// 0x00630558
void ACanvas24::PutPixelNoClip(int nX, int nY, unsigned int nColor) {
    unsigned char rgb[kChannelCount];
    ChannelsFrom8888(nColor, rgb);
    PutPixelRGBNoClip(nX, nY, rgb);
}

// 0x006303f8. For this format the native word is an 8888 colour, so the store forwards to the
// colourless slot through the table rather than converting.
void ACanvas24::PutPixelNativeNoClip(int nX, int nY, unsigned int nColor) {
    PutPixelNoClip(nX, nY, nColor);
}

// 0x006301b0
int ACanvas24::GetPixelIndexedNoClip(int nX, int nY) {
    APalette *pPalette = ResolvePalette(mBitmap.mPalette);
    if (pPalette == nullptr) {
        return 0;
    }
    unsigned char rgb[kChannelCount];
    GetPixelRGBNoClip(nX, nY, rgb);
    return IndexForChannels(pPalette, rgb);
}

// 0x00630598
unsigned short ACanvas24::GetPixel15NoClip(int nX, int nY) {
    unsigned char rgb[kChannelCount];
    GetPixelRGBNoClip(nX, nY, rgb);
    return Color15FromChannels(rgb);
}

// 0x006305f0
unsigned int ACanvas24::GetPixelNoClip(int nX, int nY) {
    unsigned char rgb[kChannelCount];
    GetPixelRGBNoClip(nX, nY, rgb);
    return Rgb8888FromChannels(rgb);
}

// 0x00630420. The read counterpart of PutPixelNativeNoClip().
unsigned int ACanvas24::GetPixelNativeNoClip(int nX, int nY) {
    return GetPixelNoClip(nX, nY);
}
