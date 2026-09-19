#include "met/metmultisaveremixscreen.h"

#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "dlg";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix. MetGlobalSettingsSaverScreen and MetRemixManager
// load the same container.
static const char *const kContainerName = "dialogue";

} // namespace

MetMultiSaveRemixScreen::MetMultiSaveRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknownd4(0) {
}
