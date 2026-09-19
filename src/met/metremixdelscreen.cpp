#include "met/metremixdelscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcrd";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_remix_del";

// The one container object the screen registers.
static const char *const kDeleteObjectName = "mem_del_remix";

} // namespace

MetRemixDelScreen::MetRemixDelScreen(MetRenderer *pRenderer, int nPriority)
    : MetSaveRemix(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknownf4(nullptr), mUnknownfc(0), mUnknown100(0), mUnknown104(0), mUnknown138(0),
      mUnknown13c(0) {
    mUnknown60 = 0;
    mUnknown38.push_back(HxStr(kDeleteObjectName));
}

MetRemixDelScreen::~MetRemixDelScreen() {
    delete mUnknownf4;
}
