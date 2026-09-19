#include "met/metsaveremixscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "ers";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "save_remix";

// The one container object the screen registers.
static const char *const kSaveObjectName = "remix_save";

} // namespace

MetSaveRemixScreen::MetSaveRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetSaveRemix(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknowne8(nullptr), mUnknownec(0), mUnknown100(0) {
    mUnknowne8 = new MetButtonList;
    mUnknown38.push_back(HxStr(kSaveObjectName));
    mUnknown5c = 0;
    mUnknowne0 = 0;
}

MetSaveRemixScreen::~MetSaveRemixScreen() {
    delete mUnknowne8;
}

void MetSaveRemixScreen::PlaySlideSound(int nSelector) {
    if (nSelector == mUnknownc8) {
        MetScreen::PlaySlideSound(nSelector);
    }
}
