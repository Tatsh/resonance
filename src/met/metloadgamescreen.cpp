#include "met/metloadgamescreen.h"

#include "msg/message.h"
#include "os/hxstr.h"

namespace {

// The screen name. It is empty in the image, which makes the two animation views resolve as
// `_EE.anim` and `_BF.anim`.
static const char *const kScreenName = "";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Transition";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "loadgame";

} // namespace

MetLoadGameScreen::MetLoadGameScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown94(0), mUnknown98(0), mUnknown9c(0), mUnknowna0(0), mUnknowna4(0), mFade(nullptr) {
    mFade = new MetFade(pRenderer);
}

MetLoadGameScreen::~MetLoadGameScreen() {
    delete mFade;
}
