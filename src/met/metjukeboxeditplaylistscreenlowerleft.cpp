#include "met/metjukeboxeditplaylistscreenlowerleft.h"

#include "os/hxstr.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbed";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_edit_data";

} // namespace

// 0x00237758
MetJukeboxEditPlaylistScreenLowerLeft::MetJukeboxEditPlaylistScreenLowerLeft(MetRenderer *pRenderer,
                                                                             int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)) {
}

// 0x0023aa40
void MetJukeboxEditPlaylistScreenLowerLeft::PlaySlideSound(int) {
}

// 0x0023aa48
void MetJukeboxEditPlaylistScreenLowerLeft::PlayLeaveSound() {
}

// 0x0023aa50
void MetJukeboxEditPlaylistScreenLowerLeft::PlayHighSound(int) {
}

// 0x0023aa58
void MetJukeboxEditPlaylistScreenLowerLeft::PlayCycleLeftSound(int) {
}

// 0x0023aa60
void MetJukeboxEditPlaylistScreenLowerLeft::PlayCycleRightSound(int) {
}

// 0x0023aa68
MetJukeboxEditPlaylistScreenLowerLeft *
MetJukeboxEditPlaylistScreenLowerLeft::New(MetRenderer *pRenderer, int nPriority) {
    return new MetJukeboxEditPlaylistScreenLowerLeft(pRenderer, nPriority);
}

// 0x0023aaf0
void MetJukeboxEditPlaylistScreenLowerLeft::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
}

// 0x0023ab10
MetJukeboxEditPlaylistScreenLowerLeft::~MetJukeboxEditPlaylistScreenLowerLeft() {
}

// 0x0023ab68
void MetJukeboxEditPlaylistScreenLowerLeft::HandleCommand(
    [[maybe_unused]] const MetScreenCommand *pCommand) {
}

// 0x0023ab70
void MetJukeboxEditPlaylistScreenLowerLeft::OnUnknownSlot33() {
}

// 0x0023ab78
void MetJukeboxEditPlaylistScreenLowerLeft::OnUnknownSlot36() {
}
