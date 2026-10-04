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

MetJukeboxEditPlaylistScreenLowerLeft::MetJukeboxEditPlaylistScreenLowerLeft(MetRenderer *pRenderer,
                                                                             int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlaySlideSound(int) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlayLeaveSound(int) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlayHighSound(int) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlayCycleLeftSound(int) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::PlayCycleRightSound(int) {
}

MetJukeboxEditPlaylistScreenLowerLeft *
MetJukeboxEditPlaylistScreenLowerLeft::New(MetRenderer *pRenderer, int nPriority) {
    return new MetJukeboxEditPlaylistScreenLowerLeft(pRenderer, nPriority);
}

void MetJukeboxEditPlaylistScreenLowerLeft::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
#ifdef VIDEO_STANDARD_PAL
    // The binary expands this loop into one call per text, and it does not test a text for null.
    for (const auto &entry : kInstructionTexts) {
        Rnd::Text *pText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(entry.pszObject)));
        pText->SetText(GetMetString(entry.nId));
    }
#endif
}

MetJukeboxEditPlaylistScreenLowerLeft::~MetJukeboxEditPlaylistScreenLowerLeft() {
}

void MetJukeboxEditPlaylistScreenLowerLeft::HandleCommand(
    [[maybe_unused]] const MetScreenCommand *pCommand) {
}

void MetJukeboxEditPlaylistScreenLowerLeft::OnEnterFinished() {
}

void MetJukeboxEditPlaylistScreenLowerLeft::OnExitFinished() {
}
