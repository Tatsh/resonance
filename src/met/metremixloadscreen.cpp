#include "met/metremixloadscreen.h"

#include "met/albumcache.h"
#include "met/metbuttonlist.h"
#include "met/metremixrecord.h"
#include "met/scrollinglist.h"
#include "os/hxstr.h"
#include "rnd/text.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcrl";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_remix_load";

// The text a row past the end of the catalogue shows.
static const char *const kNoText = "";

} // namespace

MetRemixLoadScreen::MetRemixLoadScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown94(nullptr), mUnknowna0(nullptr), mUnknowna4(nullptr), mUnknowna8(nullptr) {
    mUnknown60 = 0;
    mUnknowna8 = new MetButtonList;
}

MetRemixLoadScreen::~MetRemixLoadScreen() {
    delete mUnknown94;
    mUnknown94 = nullptr;
    delete mUnknowna8;
}

void MetRemixLoadScreen::PlaySlideSound(int nSelector) {
    if (mUnknown90 != nullptr && mUnknown90->size() != 0) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

void MetRemixLoadScreen::PlayHighSound(int nSelector) {
    if (mUnknown90 != nullptr && mUnknown90->size() != 0) {
        MetScreen::PlayHighSound(nSelector);
    }
}

int MetRemixLoadScreen::ProvideText(int nItem, int, Rnd::Text *pText, int) {
    // The catalogue pointer is not tested for null here, unlike in the two sound overrides.
    if (static_cast<unsigned>(nItem) < mUnknown90->size()) {
        MetRemixRecord record((*mUnknown90)[nItem]);
        HxStr name(record.unknown08_);
        pText->SetText(name);
        pText->SetFont(record.unknown34_ == GetAlbumJukeboxValue() ? mUnknowna0 : mUnknowna4);
    } else {
        pText->SetText(HxStr(kNoText));
    }
    return 1;
}

int MetRemixLoadScreen::ProvideMesh(int, int, Rnd::Mesh *, int) {
    return 1;
}
