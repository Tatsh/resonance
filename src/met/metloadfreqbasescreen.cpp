#include "met/metloadfreqbasescreen.h"

#include "game/personatexture.h"
#include "met/metfreqmakerassetmanager.h"
#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "cid";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "create_id";

// The burn texture the constructor resolves.
constexpr int kBurnTextureIndex = 0;

// Entries the identity list needs before either cycle sound plays.
constexpr unsigned kMinimumCyclableEntries = 2;

// MetButtonList::mSelected while the identity carousel rather than a button is selected.
constexpr int kCarouselSelected = 0;

} // namespace

MetLoadFreqBaseScreen::MetLoadFreqBaseScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mUnknown94 = 0;
    mUnknown90 = new MetButtonList;
    MetFreqMakerAssetManager::shared()->WaitForLoad();
    // Yes, the binary polls once more after the wait has already run the load to completion, and
    // discards the result.
    MetFreqMakerAssetManager::shared()->PollLoad();
    mBurnTexture = findPersonaBurnTexture(kBurnTextureIndex);
}

MetLoadFreqBaseScreen::~MetLoadFreqBaseScreen() {
    delete mUnknown90;
}

void MetLoadFreqBaseScreen::PlayCycleLeftSound(int nSelector) {
    if (mUnknown90->mSelected == kCarouselSelected &&
        mUnknown8c->size() >= kMinimumCyclableEntries) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

void MetLoadFreqBaseScreen::PlayCycleRightSound(int nSelector) {
    if (mUnknown90->mSelected == kCarouselSelected &&
        mUnknown8c->size() >= kMinimumCyclableEntries) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}
