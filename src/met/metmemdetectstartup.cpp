#include "met/metmemdetectstartup.h"

#include "msg/message.h"
#include "os/hxstr.h"

namespace {

// The screen name. It is empty in the image, which makes the two animation views resolve as
// `_EE.anim` and `_BF.anim`.
static const char *const kScreenName = "";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memdetect1";

} // namespace

MetMemDetectStartup::MetMemDetectStartup(MetRenderer *pRenderer, int nPriority)
    : MetMemDetectScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mFade(nullptr), mUnknowna8(0) {
    mFade = new MetFade(pRenderer);
}

MetMemDetectStartup::~MetMemDetectStartup() {
    delete mFade;
}
