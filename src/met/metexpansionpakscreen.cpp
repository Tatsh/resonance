#include "met/metexpansionpakscreen.h"

#include "met/metfade.h"
#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "dlg";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "dialogue";

// The two message screens the state machine drives.
static const char *const kPrepareMessage = "expansion_prepare";
static const char *const kLoadMessage = "expansion_load";

} // namespace

MetExpansionPakScreen::MetExpansionPakScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mFade(nullptr) {
    mFade = new MetFade(pRenderer);
}

void MetExpansionPakScreen::OnMsgScreenShown(const HxStr &name) {
    if (name == kPrepareMessage) {
        mUnknownac = 1;
        return;
    }

    if (name == kLoadMessage) {
        mUnknownb0 = 1;
    }
}

void MetExpansionPakScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
}
