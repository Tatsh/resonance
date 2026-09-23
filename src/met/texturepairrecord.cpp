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

// 0x00246de0
TexturePairRecord::TexturePairRecord(const HxStr &first, const HxStr &second)
    : mCurrent(0), mPending(1), mLoading(0), mResolved(0), mFirstName(first), mSecondName(second),
      mInvalid(0) {
}

// 0x001fc568
TexturePairRecord::~TexturePairRecord() {
}

// 0x002498c0
void TexturePairRecord::invalidate() {
    mInvalid = 1;
}

// 0x00246ef0
void TexturePairRecord::ResolveTextures() {
    if (mResolved) {
        return;
    }
    mResolved = 1;
    mTextures.push_back(dynamic_cast<Rnd::Tex *>(Rnd::g_manager.Find(mFirstName)));
    mTextures.push_back(dynamic_cast<Rnd::Tex *>(Rnd::g_manager.Find(mSecondName)));
}

// 0x00249760
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

// 0x00249808
Rnd::Tex *TexturePairRecord::Current() {
    if (mInvalid) {
        return nullptr;
    }
    ResolveTextures();
    return mTextures[mCurrent];
}

// 0x00249850
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

// 0x00247038
HxStr TexturePairRecord::LogoPath(const HxStr &name) {
    return BuildBitmapPath(
        name, "\\MetaGame\\shared\\rand_logo.bmp", "\\Levels\\%s\\images\\%s_logo.bmp");
}

// 0x00247250
HxStr TexturePairRecord::PicturePath(const HxStr &name) {
    return BuildBitmapPath(
        name, "\\MetaGame\\shared\\rand_pic.bmp", "\\Levels\\%s\\images\\%s_pic.bmp");
}

// 0x00247468
HxStr TexturePairRecord::ArenaPath(const HxStr &name) {
    return BuildBitmapPath(name, "\\MetaGame\\shared\\rand_pic.bmp", "\\Arenas\\%s\\as_%s.bmp");
}
