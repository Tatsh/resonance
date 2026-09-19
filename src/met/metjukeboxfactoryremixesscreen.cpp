#include "met/metjukeboxfactoryremixesscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbf";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_factory";

// mUnknownc8 for the factory list, where the other two jukebox screens clear the same member.
constexpr int kFactoryListSelector = -1;

} // namespace

MetJukeboxFactoryRemixesScreen::MetJukeboxFactoryRemixesScreen(MetRenderer *pRenderer,
                                                               int nPriority)
    : MetJukeboxBaseScreen(pRenderer,
                           nPriority,
                           HxStr(kScreenName),
                           HxStr(kContainerDirectory),
                           HxStr(kContainerFile)) {
    mUnknownc8 = kFactoryListSelector;
}

MetJukeboxFactoryRemixesScreen::~MetJukeboxFactoryRemixesScreen() {
}

int MetJukeboxFactoryRemixesScreen::GetItemCount() {
    return mUnknowna0->size();
}
