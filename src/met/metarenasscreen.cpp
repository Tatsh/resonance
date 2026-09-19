#include "met/metarenasscreen.h"

#include "met/metbuttonlist.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/texturepairrecord.h"
#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "as";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "arena_sel";

// The extra container object the constructor asks MetScreen to resolve.
static const char *const kArenasObject = "arenas";

} // namespace

MetArenasScreen::MetArenasScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(new MetButtonList()), mUnknown90(nullptr), mUnknown9c(nullptr) {
    mUnknown38.push_back(HxStr(kArenasObject));
}

MetArenasScreen::~MetArenasScreen() {
    delete mUnknown8c;
    delete mUnknown90;
}

void MetArenasScreen::PlaySlideSound(int nSelector) {
    if (mUnknown8c->mSelected < mUnknown98 || mUnknown8c->mSelected == mUnknown94) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

void MetArenasScreen::PlayCycleLeftSound(int) {
}

void MetArenasScreen::PlayCycleRightSound(int) {
}

void MetArenasScreen::OnUnknownSlot33() {
    MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
}
