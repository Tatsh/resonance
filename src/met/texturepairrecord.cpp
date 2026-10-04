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
        path = MakeFreqPath(HxStr(Rnd::MakeString(pszFormat, pszName, pszName)));
    }
    return path;
}

} // namespace

TexturePairRecord::TexturePairRecord(const HxStr &first, const HxStr &second)
    : mCurrent(0), mPending(1), mLoading(0), mResolved(0), mFirstName(first), mSecondName(second),
      mInvalid(0) {
}

TexturePairRecord::~TexturePairRecord() {
}

void TexturePairRecord::invalidate() {
    mInvalid = 1;
}

void TexturePairRecord::ResolveTextures() {
    if (mResolved) {
        return;
    }
    mResolved = 1;
    mTextures.push_back(dynamic_cast<Rnd::Tex *>(Rnd::TheManager.Find(mFirstName)));
    mTextures.push_back(dynamic_cast<Rnd::Tex *>(Rnd::TheManager.Find(mSecondName)));
}

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

void TexturePairRecord::CancelLoad() {
    mLoading = 0;
}

Rnd::Tex *TexturePairRecord::Current() {
    if (mInvalid) {
        return nullptr;
    }
    ResolveTextures();
    return mTextures[mCurrent];
}

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

HxStr TexturePairRecord::LogoPath(const HxStr &name) {
    return BuildBitmapPath(
        name, "\\MetaGame\\shared\\rand_logo.bmp", "\\Levels\\%s\\images\\%s_logo.bmp");
}

HxStr TexturePairRecord::PicturePath(const HxStr &name) {
    return BuildBitmapPath(
        name, "\\MetaGame\\shared\\rand_pic.bmp", "\\Levels\\%s\\images\\%s_pic.bmp");
}

HxStr TexturePairRecord::ArenaPath(const HxStr &name) {
    return BuildBitmapPath(name, "\\MetaGame\\shared\\rand_pic.bmp", "\\Arenas\\%s\\as_%s.bmp");
}
