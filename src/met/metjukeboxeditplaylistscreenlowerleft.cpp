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
void MetJukeboxEditPlaylistScreenLowerLeft::PlayLeaveSound(int) {
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

// 0x0023ab10
MetJukeboxEditPlaylistScreenLowerLeft::~MetJukeboxEditPlaylistScreenLowerLeft() {
}

// 0x0023ab68
void MetJukeboxEditPlaylistScreenLowerLeft::HandleCommand(
    [[maybe_unused]] const MetScreenCommand *pCommand) {
}

// 0x0023ab70
void MetJukeboxEditPlaylistScreenLowerLeft::OnEnterFinished() {
}

// 0x0023ab78
void MetJukeboxEditPlaylistScreenLowerLeft::OnExitFinished() {
}
