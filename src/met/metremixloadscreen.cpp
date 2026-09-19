#include "met/metremixloadscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcrl";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_remix_load";

} // namespace

MetRemixLoadScreen::MetRemixLoadScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown94(nullptr), mUnknowna0(0), mUnknowna4(0), mUnknowna8(nullptr) {
    mUnknown60 = 0;
    mUnknowna8 = new MetButtonList;
}

MetRemixLoadScreen::~MetRemixLoadScreen() {
    delete mUnknown94;
    mUnknown94 = nullptr;
    delete mUnknowna8;
}
