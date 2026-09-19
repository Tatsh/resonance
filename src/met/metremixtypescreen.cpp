#include "met/metremixtypescreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "smrt";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "sm_remixtype";

// The three container objects the screen registers, in the order the constructor pushes them.
static const char *const kNewObjectName = "smrt_new";
static const char *const kLoadObjectName = "smrt_load";
static const char *const kJukeboxObjectName = "smrt_jukebox";

} // namespace

MetRemixTypeScreen::MetRemixTypeScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(nullptr) {
    mUnknown38.push_back(HxStr(kNewObjectName));
    mUnknown38.push_back(HxStr(kLoadObjectName));
    mUnknown38.push_back(HxStr(kJukeboxObjectName));
    mUnknown8c = new MetButtonList;
}

MetRemixTypeScreen::~MetRemixTypeScreen() {
    delete mUnknown8c;
}
