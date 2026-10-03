#include "met/metpausesologamescreen.h"

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

static const char *const kScreenName = "psgr";
static const char *const kDirectory = "metagame/Transition";
static const char *const kContainerName = "pause_solo";
static const char *const kPanelName = "MetPauseSoloGameScreen";

// Counted from 1.
static const char *const kOptionTextFormat = "psgr_opt%d.txt";
static const char *const kPausedText = "psgr_paused.txt";

static const char *const kRemixHeadingKey = "pause_remix";
static const char *const kGameHeadingKey = "pause_game";
static const char *const kGameLabelsKey = "pause_solo_game";
static const char *const kRemixLabelsKey = "pause_solo_remix";

static const char *const kNoText = "";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kControllerScreen = "MetConfigControllerScreen";
static const char *const kGameOptionsScreen = "MetConfigGameOptionsScreen";
static const char *const kHelpScreen = "MetHelpScreen";

constexpr int kOptionCount = 5;

constexpr int kPromptConfigCode = 0x258;
constexpr int kLabelsConfigCode = 0x259;

// Command codes past MetScreenCommandCode's range. Their names are inferred from the screens
// they open.
constexpr int kCommandGameOptions = 7;
constexpr int kCommandController = 8;
constexpr int kCommandIgnored = 9;
constexpr int kCommandResume = 10;

} // namespace

// NTSC-U/C: 0x0031fd98, PAL: 0x00346768
MetPauseSoloGameScreen::MetPauseSoloGameScreen(MetRenderer *pRenderer, int nPriority)
    : MetPauseBaseScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mLeavingForConfig(0) {
    mReturnPanel = kPanelName;
}

// NTSC-U/C: 0x0031ff38, PAL: 0x00346970
void MetPauseSoloGameScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    for (int i = 1; i <= kOptionCount; ++i) {
        Rnd::Text *pOption = dynamic_cast<Rnd::Text *>(
            Rnd::TheManager.Find(HxStr(Rnd::MakeString(kOptionTextFormat, i))));
        mOptionTexts.push_back(pOption);
    }
}

// NTSC-U/C: 0x00320088, PAL: 0x00346ae8
void MetPauseSoloGameScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandSelect:
    case kMetScreenCommandBack:
    case kCommandResume:
        MetPauseBaseScreen::HandleCommand(pCommand);
        return;
    case kCommandController:
        PlayPauseSound(pCommand->mPadIndex);
        mLeavingForConfig = 1;
        mExitAction = kExitController;
        break;
    case kCommandGameOptions:
        PlayPauseSound(pCommand->mPadIndex);
        mExitAction = kExitGameOptions;
        mLeavingForConfig = 1;
        break;
    case kCommandIgnored:
    default:
        return;
    }
    ActivateNamedPanel(HxStr(kNoText));
    BeginExit();
}

// NTSC-U/C: 0x00320230, PAL: 0x00346cc0
void MetPauseSoloGameScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Rnd::Text *pPaused = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kPausedText)));

    HxStr heading = params.mPlayMode == kPlayModeJam ?
                        MetConfigText(kMetStrPauseRemix, kPromptConfigCode, kRemixHeadingKey) :
                        MetConfigText(kMetStrPauseGame, kPromptConfigCode, kGameHeadingKey);
    pPaused->SetText(heading);

    mOptionLabels.clear();
#ifdef VIDEO_STANDARD_PAL
    mOptionLabels.push_back(GetMetString(kMetStrPauseResume));
    mOptionLabels.push_back(GetMetString(kMetStrPauseQuit));
    if (params.mPlayMode == kPlayModeGame) {
        mOptionLabels.push_back(GetMetString(kMetStrPauseRestart));
    }
    mOptionLabels.push_back(GetMetString(kMetStrPauseConfig));
    mOptionLabels.push_back(GetMetString(kMetStrPauseSettings));
#else
    QueryConfigStrings(&mOptionLabels,
                       kLabelsConfigCode,
                       params.mPlayMode == kPlayModeGame ? kGameLabelsKey : kRemixLabelsKey);
#endif
    // Yes, the binary copies the labels here and again in the base slot.
    for (int i = 0; i < kOptionCount; ++i) {
        mOptionTexts[i]->SetText(mOptionLabels[i]);
    }
    MetPauseBaseScreen::EnterAndShow();
    mLeavingForConfig = 0;
}

// NTSC-U/C: 0x003205a8, PAL: 0x00347628
void MetPauseSoloGameScreen::OnExitFinished() {
    if (mLeavingForConfig == 0) {
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
    PushNamedScreen(HxStr(kTitleScreen));
    PushNamedScreen(HxStr(pszConfigScreen));
    PushNamedScreen(HxStr(kHelpScreen));
    ActivateNamedPanel(HxStr(pszConfigScreen));
}

// NTSC-U/C: 0x00323a60, PAL: 0x0034afd0
MetPauseSoloGameScreen *MetPauseSoloGameScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPauseSoloGameScreen(pRenderer, nPriority);
}
