#include "met/metjukeboxeditplaylistscreenlowerleft.h"

#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbed";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_edit_data";

#ifdef VIDEO_STANDARD_PAL
// Each instruction text and the identifier it is filled from, in the order slot 38 fills them.
struct InstructionText {
    const char *pszObject;
    MetStringId nId;
};

static const InstructionText kInstructionTexts[] = {
    {"jbed_CREATE PLAYLIST.txt", kMetStrJbedInstruct},
    {"jbed_l1l2_val.txt", kMetStrJbedSort},
    {"jbed_x_val.txt", kMetStrJbedDel},
    {"jbed_square_val.txt", kMetStrJbedClear},
};
#endif

} // namespace

// NTSC-U/C: 0x00237758, PAL: 0x0024b7b8
MetJukeboxEditPlaylistScreenLowerLeft::MetJukeboxEditPlaylistScreenLowerLeft(MetRenderer *pRenderer,
                                                                             int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)) {
}

// NTSC-U/C: 0x0023aa40, PAL: 0x0024f0d0
void MetJukeboxEditPlaylistScreenLowerLeft::PlaySlideSound(int) {
}

// NTSC-U/C: 0x0023aa48, PAL: 0x0024f0d8
void MetJukeboxEditPlaylistScreenLowerLeft::PlayLeaveSound(int) {
}

// NTSC-U/C: 0x0023aa50, PAL: 0x0024f0e0
void MetJukeboxEditPlaylistScreenLowerLeft::PlayHighSound(int) {
}

// NTSC-U/C: 0x0023aa58, PAL: 0x0024f0e8
void MetJukeboxEditPlaylistScreenLowerLeft::PlayCycleLeftSound(int) {
}

// NTSC-U/C: 0x0023aa60, PAL: 0x0024f0f0
void MetJukeboxEditPlaylistScreenLowerLeft::PlayCycleRightSound(int) {
}

// NTSC-U/C: 0x0023aa68, PAL: 0x0024f0f8
MetJukeboxEditPlaylistScreenLowerLeft *
MetJukeboxEditPlaylistScreenLowerLeft::New(MetRenderer *pRenderer, int nPriority) {
    return new MetJukeboxEditPlaylistScreenLowerLeft(pRenderer, nPriority);
}

// NTSC-U/C: 0x0023aaf0, PAL: 0x0024b988
void MetJukeboxEditPlaylistScreenLowerLeft::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
#ifdef VIDEO_STANDARD_PAL
    // The binary expands this loop into one call per text, and it does not test a text for null.
    for (const auto &entry : kInstructionTexts) {
        Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(entry.pszObject)));
        pText->SetText(GetMetString(entry.nId));
    }
#endif
}

// NTSC-U/C: 0x0023ab10, PAL: 0x0024f180
MetJukeboxEditPlaylistScreenLowerLeft::~MetJukeboxEditPlaylistScreenLowerLeft() {
}

// NTSC-U/C: 0x0023ab68, PAL: 0x0024f1d8
void MetJukeboxEditPlaylistScreenLowerLeft::HandleCommand(
    [[maybe_unused]] const MetScreenCommand *pCommand) {
}

// NTSC-U/C: 0x0023ab70, PAL: 0x0024f1e0
void MetJukeboxEditPlaylistScreenLowerLeft::OnEnterFinished() {
}

// NTSC-U/C: 0x0023ab78, PAL: 0x0024f1e8
void MetJukeboxEditPlaylistScreenLowerLeft::OnExitFinished() {
}
