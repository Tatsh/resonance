#include "met/metjukeboxeditplaylistscreendone.h"

#include "met/metbuttonlist.h"
#include "os/hxstr.h"

namespace {

// The screen name, the container directory, and the container.
static const char *const kScreenName = "jbd";
static const char *const kContainerDirectory = "metagame/Shared";
static const char *const kContainerFile = "juke_done_butts";

} // namespace

MetJukeboxEditPlaylistScreenDone::MetJukeboxEditPlaylistScreenDone(MetRenderer *pRenderer,
                                                                   int nPriority)
    : MetScreen(pRenderer,
                nPriority,
                HxStr(kScreenName),
                HxStr(kContainerDirectory),
                HxStr(kContainerFile)),
      // Yes, the binary clears the member and then overwrites it in the body below.
      mUnknown8c(nullptr), mUnknown90(0), mUnknown94(0), mUnknown98(0) {
    mUnknown8c = new MetButtonList;
}

MetJukeboxEditPlaylistScreenDone::~MetJukeboxEditPlaylistScreenDone() {
}

void MetJukeboxEditPlaylistScreenDone::PlaySlideSound(int) {
}

void MetJukeboxEditPlaylistScreenDone::PlayLeaveSound() {
}

void MetJukeboxEditPlaylistScreenDone::PlayHighSound(int) {
}

void MetJukeboxEditPlaylistScreenDone::PlayCycleLeftSound(int) {
}

void MetJukeboxEditPlaylistScreenDone::PlayCycleRightSound(int) {
}
