#include "met/metjukeboxcustomremixesscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbs";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_saved";

} // namespace

MetJukeboxCustomRemixesScreen::MetJukeboxCustomRemixesScreen(MetRenderer *pRenderer, int nPriority)
    : MetJukeboxBaseScreen(pRenderer,
                           nPriority,
                           HxStr(kScreenName),
                           HxStr(kContainerDirectory),
                           HxStr(kContainerFile)) {
    mUnknownc8 = 0;
}

MetJukeboxCustomRemixesScreen::~MetJukeboxCustomRemixesScreen() {
}
