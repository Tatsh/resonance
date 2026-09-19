#include "met/metjukeboxeditplaylistscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbep";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_edit_playlist";

} // namespace

MetJukeboxEditPlaylistScreen::MetJukeboxEditPlaylistScreen(MetRenderer *pRenderer, int nPriority)
    : MetJukeboxBaseScreen(pRenderer,
                           nPriority,
                           HxStr(kScreenName),
                           HxStr(kContainerDirectory),
                           HxStr(kContainerFile)) {
    mUnknownc8 = 0;
}

MetJukeboxEditPlaylistScreen::~MetJukeboxEditPlaylistScreen() {
}

int MetJukeboxEditPlaylistScreen::GetItemCount() {
    return mUnknownc4->entries.size();
}
