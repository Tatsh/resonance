#include "met/metmemcardloadscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcl";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_load";

// The one container object the screen registers.
static const char *const kCardObjectName = "mcl_card";

// Cards the list needs before either cycle sound plays.
constexpr unsigned kMinimumCyclableCards = 2;

} // namespace

MetMemCardLoadScreen::MetMemCardLoadScreen(MetRenderer *pRenderer, int nPriority)
    : MetMemDetectScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknowna4(0), mUnknowna8(0), mUnknownd0(0), mUnknownd4(0), mUnknowne4(0) {
    mUnknown38.push_back(HxStr(kCardObjectName));
}

void MetMemCardLoadScreen::PlaySlideSound(int nSelector) {
    if (!mCards.empty()) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

void MetMemCardLoadScreen::PlayCycleLeftSound(int nSelector) {
    if (mCards.size() >= kMinimumCyclableCards) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

void MetMemCardLoadScreen::PlayCycleRightSound(int nSelector) {
    if (mCards.size() >= kMinimumCyclableCards) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}
