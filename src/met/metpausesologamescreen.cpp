#include "met/metpausesologamescreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metfrontendstate.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

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

// 0x0031fd98
MetPauseSoloGameScreen::MetPauseSoloGameScreen(MetRenderer *pRenderer, int nPriority)
    : MetPauseBaseScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknownb0(0) {
    mUnknown9c = kPanelName;
}

// 0x0031ff38
void MetPauseSoloGameScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    for (int i = 1; i <= kOptionCount; ++i) {
        Rnd::Text *pOption = dynamic_cast<Rnd::Text *>(
            Rnd::g_manager.Find(HxStr(FormatString(kOptionTextFormat, i))));
        mUnknowna4.push_back(pOption);
    }
}

// 0x00320088
void MetPauseSoloGameScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandSelect:
    case kMetScreenCommandBack:
    case kCommandResume:
        MetPauseBaseScreen::HandleCommand(pCommand);
        return;
    case kCommandController:
        PlayPauseSound(pCommand->mPadIndex);
        mUnknownb0 = 1;
        mUnknown8c = kExitController;
        break;
    case kCommandGameOptions:
        PlayPauseSound(pCommand->mPadIndex);
        mUnknown8c = kExitGameOptions;
        mUnknownb0 = 1;
        break;
    case kCommandIgnored:
    default:
        return;
    }
    ActivateNamedPanel(HxStr(kNoText));
    BeginExit();
}

// 0x00320230
void MetPauseSoloGameScreen::EnterAndShow() {
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    Rnd::Text *pPaused = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kPausedText)));

    HxStr heading = QueryConfigString(
        kPromptConfigCode, params.mUnknown1c == kPlayModeJam ? kRemixHeadingKey : kGameHeadingKey);
    pPaused->SetText(heading);

    mUnknown90.clear();
    QueryConfigStrings(&mUnknown90,
                       kLabelsConfigCode,
                       params.mUnknown1c == kPlayModeGame ? kGameLabelsKey : kRemixLabelsKey);
    // Yes, the binary copies the labels here and again in the base slot.
    for (int i = 0; i < kOptionCount; ++i) {
        mUnknowna4[i]->SetText(mUnknown90[i]);
    }
    MetPauseBaseScreen::EnterAndShow();
    mUnknownb0 = 0;
}

// 0x003205a8
void MetPauseSoloGameScreen::OnUnknownSlot36() {
    if (mUnknownb0 == 0) {
        MetPauseBaseScreen::OnUnknownSlot36();
        return;
    }

    const char *pszConfigScreen;
    if (mUnknown8c == kExitController) {
        pszConfigScreen = kControllerScreen;
    } else if (mUnknown8c == kExitGameOptions) {
        pszConfigScreen = kGameOptionsScreen;
    } else {
        return;
    }
    MetFrontEndState::shared()->mUnknown24 = HxStr(kPanelName);
    PushNamedScreen(HxStr(kTitleScreen));
    PushNamedScreen(HxStr(pszConfigScreen));
    PushNamedScreen(HxStr(kHelpScreen));
    ActivateNamedPanel(HxStr(pszConfigScreen));
}

// 0x00323a60
MetPauseSoloGameScreen *MetPauseSoloGameScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPauseSoloGameScreen(pRenderer, nPriority);
}
