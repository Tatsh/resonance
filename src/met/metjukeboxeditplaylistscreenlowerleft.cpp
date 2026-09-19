#include "met/metjukeboxeditplaylistscreenlowerleft.h"

#include "os/hxstr.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbed";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_edit_data";

} // namespace

MetJukeboxEditPlaylistScreenLowerLeft::MetJukeboxEditPlaylistScreenLowerLeft(MetRenderer *pRenderer,
                                                                             int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)) {
}

MetJukeboxEditPlaylistScreenLowerLeft::~MetJukeboxEditPlaylistScreenLowerLeft() {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlaySlideSound(int) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlayLeaveSound() {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlayHighSound(int) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlayCycleLeftSound(int) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlayCycleRightSound(int) {
}
