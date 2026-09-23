#include "met/metglobalsettingssaverscreen.h"

#include "met/metfrontendstate.h"
#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "dlg";
// The directory the container loads from. The capital S is what the image records.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "dialogue";

// The registry keys StartSave() resolves.
static const char *const kOwnScreenName = "MetGlobalSettingsSaverScreen";
static const char *const kSonyScreen = "MetSonyScreen";

} // namespace

MetGlobalSettingsSaverScreen::MetGlobalSettingsSaverScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
}

void MetGlobalSettingsSaverScreen::StartSave(const std::vector<HxStr> &screens) {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    MetGlobalSettingsSaverScreen *pSaver =
        pScreen != nullptr ? dynamic_cast<MetGlobalSettingsSaverScreen *>(pScreen) : nullptr;
    // The binary does not test the result for null.
    pSaver->SetReturnScreens(screens);

    MetScreen *pSony = MetScreen::FindScreenByName(HxStr(kSonyScreen));
    if (MetFrontEndState::shared()->mUnknown0c != 0) {
        pSony->PushNamedScreen(HxStr(kOwnScreenName));
        return;
    }
    int nCount = screens.size();
    for (int i = 0; i < nCount; ++i) {
        pSony->PushNamedScreen(screens[i]);
    }
    pSony->ActivateNamedPanel(screens[0]);
}

void MetGlobalSettingsSaverScreen::SetReturnScreens(const std::vector<HxStr> &screens) {
    mUnknown90 = screens;
}
