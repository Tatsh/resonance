#include "met/texturepairrecord.h"

#include "os/formatstring.h"
#include "os/hostmode.h"
#include "os/zone.h"
#include "rnd/manager.h"
#include "rnd/tex.h"

namespace {

// The name that selects the shared random bitmap.
constexpr char kRandomName[] = "random";

// The depth Load() configures every bitmap with.
constexpr int kBitmapBitsPerPixel = 16;

// Builds a path from one of two templates, the fixed one for the name `random` and the formatted
// one, with the name twice, for every other name.
inline HxStr BuildBitmapPath(const HxStr &name, const char *pszRandomPath, const char *pszFormat) {
    HxStr path("");
    if (name == kRandomName) {
        path = MakeFreqPath(HxStr(pszRandomPath));
    } else {
        const char *pszName = name.mStr != nullptr ? name.mStr : g_szEmptyString;
        path = MakeFreqPath(HxStr(FormatString(pszFormat, pszName, pszName)));
    }
    return path;
}

} // namespace

// NTSC-U/C: 0x00246de0, PAL: 0x0025bf20
TexturePairRecord::TexturePairRecord(const HxStr &first, const HxStr &second)
    : mCurrent(0), mPending(1), mLoading(0), mResolved(0), mFirstName(first), mSecondName(second),
      mInvalid(0) {
}

// NTSC-U/C: 0x001fc568, PAL: 0x002039d0
TexturePairRecord::~TexturePairRecord() {
}

// NTSC-U/C: 0x002498c0, PAL: 0x0025ebd0
void TexturePairRecord::invalidate() {
    mInvalid = 1;
}

// NTSC-U/C: 0x00246ef0, PAL: 0x0025c040
void TexturePairRecord::ResolveTextures() {
    if (mResolved) {
        return;
    }
    mResolved = 1;
    mTextures.push_back(dynamic_cast<Rnd::Tex *>(Rnd::TheManager.Find(mFirstName)));
    mTextures.push_back(dynamic_cast<Rnd::Tex *>(Rnd::TheManager.Find(mSecondName)));
}

// NTSC-U/C: 0x00249760, PAL: 0x0025ea70
void TexturePairRecord::Load(const HxStr &path) {
    ResolveTextures();
    mLoading = 1;
    Rnd::Tex *pTex = mTextures[mPending];
    pTex->SetBitmapConfig(0, 0, kBitmapBitsPerPixel, path, pTex->mMipSelect, pTex->mFlags);
    const int nZone = ZoneGetCurrent();
    ZoneSetCurrent(kNoZone);
    pTex->ReloadBitmaps();
    ZoneSetCurrent(nZone);
}

// NTSC-U/C: 0x00249800, PAL: 0x0025eb10
void TexturePairRecord::CancelLoad() {
    mLoading = 0;
}

// NTSC-U/C: 0x00249808, PAL: 0x0025eb18
Rnd::Tex *TexturePairRecord::Current() {
    if (mInvalid) {
        return nullptr;
    }
    ResolveTextures();
    return mTextures[mCurrent];
}

// NTSC-U/C: 0x00249850, PAL: 0x0025eb60
int TexturePairRecord::Advance() {
    if (!mLoading) {
        return 0;
    }
    if (!mTextures[mPending]->PollAsyncMips()) {
        return 0;
    }
    const int nPending = mPending;
    mPending = mCurrent;
    mCurrent = nPending;
    mInvalid = 0;
    mLoading = 0;
    return 1;
}

// NTSC-U/C: 0x00247038, PAL: 0x0025c188
HxStr TexturePairRecord::LogoPath(const HxStr &name) {
    return BuildBitmapPath(
        name, "\\MetaGame\\shared\\rand_logo.bmp", "\\Levels\\%s\\images\\%s_logo.bmp");
}

// NTSC-U/C: 0x00247250, PAL: 0x0025c428
HxStr TexturePairRecord::PicturePath(const HxStr &name) {
    return BuildBitmapPath(
        name, "\\MetaGame\\shared\\rand_pic.bmp", "\\Levels\\%s\\images\\%s_pic.bmp");
}

// NTSC-U/C: 0x00247468, PAL: 0x0025c6c8
HxStr TexturePairRecord::ArenaPath(const HxStr &name) {
    return BuildBitmapPath(name, "\\MetaGame\\shared\\rand_pic.bmp", "\\Arenas\\%s\\as_%s.bmp");
}
