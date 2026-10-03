#include "met/metpausesoloremixscreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metfrontendstate.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"

namespace {

static const char *const kScreenName = "psr";
static const char *const kDirectory = "metagame/Transition";
static const char *const kContainerName = "pause_soloremix";
static const char *const kPanelName = "MetPauseSoloRemixScreen";

// Counted from 1.
static const char *const kOptionTextFormat = "psr_opt%d.txt";
static const char *const kPausedText = "psr_paused.txt";

static const char *const kHeadingKey = "pause_remix";
static const char *const kLabelsKey = "pause_solo_remix";

static const char *const kNoText = "";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kControllerScreen = "MetConfigControllerScreen";
static const char *const kGameOptionsScreen = "MetConfigGameOptionsScreen";
static const char *const kHelpScreen = "MetHelpScreen";

constexpr int kOptionCount = 4;

constexpr int kPromptConfigCode = 0x258;
constexpr int kLabelsConfigCode = 0x259;

// Command codes past MetScreenCommandCode's range. Their names are inferred from the screens
// they open.
constexpr int kCommandGameOptions = 7;
constexpr int kCommandController = 8;
constexpr int kCommandResume = 10;

} // namespace

// NTSC-U/C: 0x00323db8, PAL: 0x0034b340
MetPauseSoloRemixScreen::MetPauseSoloRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetPauseBaseScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mOpensConfigScreen(0) {
    mReturnPanel = kPanelName;
}

// NTSC-U/C: 0x00323f58, PAL: 0x0034b548
void MetPauseSoloRemixScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    for (int i = 1; i <= kOptionCount; ++i) {
        Rnd::Text *pOption = dynamic_cast<Rnd::Text *>(
            Rnd::TheManager.Find(HxStr(FormatString(kOptionTextFormat, i))));
        mOptionTexts.push_back(pOption);
    }
}

// NTSC-U/C: 0x003240a8, PAL: 0x0034b6c0
void MetPauseSoloRemixScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandBack:
    case kCommandResume:
        MetPauseBaseScreen::HandleCommand(pCommand);
        return;
    case kCommandController:
        PlayPauseSound(pCommand->mPadIndex);
        mOpensConfigScreen = 1;
        mExitAction = kExitController;
        break;
    case kCommandGameOptions:
        PlayPauseSound(pCommand->mPadIndex);
        mExitAction = kExitGameOptions;
        mOpensConfigScreen = 1;
        break;
    default:
        return;
    }
    ActivateNamedPanel(HxStr(kNoText));
    BeginExit();
}

// NTSC-U/C: 0x00324260, PAL: 0x0034b8a8
void MetPauseSoloRemixScreen::EnterAndShow() {
    // Yes, the binary copies the settings and never reads the copy.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Rnd::Text *pPaused = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kPausedText)));

    HxStr heading = MetConfigText(kMetStrPauseRemix, kPromptConfigCode, kHeadingKey);
    pPaused->SetText(heading);

    mOptionLabels.clear();
#ifdef VIDEO_STANDARD_PAL
    mOptionLabels.push_back(GetMetString(kMetStrPauseResume));
    mOptionLabels.push_back(GetMetString(kMetStrPauseQuit));
    mOptionLabels.push_back(GetMetString(kMetStrPauseConfig));
    mOptionLabels.push_back(GetMetString(kMetStrPauseSettings));
#else
    QueryConfigStrings(&mOptionLabels, kLabelsConfigCode, kLabelsKey);
#endif
    // Yes, the binary copies the labels here and again in the base slot.
    for (std::vector<HxStr>::size_type i = 0; i < mOptionLabels.size(); ++i) {
        mOptionTexts[i]->SetText(mOptionLabels[i]);
    }
    MetPauseBaseScreen::EnterAndShow();
    mOpensConfigScreen = 0;
}

// NTSC-U/C: 0x00324580, PAL: 0x0034bec8
void MetPauseSoloRemixScreen::OnExitFinished() {
    if (mOpensConfigScreen == 0) {
        MetPauseBaseScreen::OnExitFinished();
        return;
    }

    const char *pszConfigScreen;
    if (mExitAction == kExitController) {
        pszConfigScreen = kControllerScreen;
    } else if (mExitAction == kExitGameOptions) {
        pszConfigScreen = kGameOptionsScreen;
    } else {
        return;
    }
    MetFrontEndState::shared()->mReturnScreen = HxStr(kPanelName);
    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kTitleScreen));
    PushNamedScreen(HxStr(pszConfigScreen));
    ActivateNamedPanel(HxStr(pszConfigScreen));
}

// NTSC-U/C: 0x00327a38, PAL: 0x0034f870
MetPauseSoloRemixScreen *MetPauseSoloRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPauseSoloRemixScreen(pRenderer, nPriority);
}
