#include "met/metjukeboxtopbuttonsscreen.h"

#include "met/metbuttonlist.h"
#include "os/hxstr.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbb";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_butts";

} // namespace

MetJukeboxTopButtonsScreen::MetJukeboxTopButtonsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)),
      mUnknown94(nullptr), mUnknown98(0) {
    mUnknown94 = new MetButtonList;
}

MetJukeboxTopButtonsScreen::~MetJukeboxTopButtonsScreen() {
}
