#include "met/metsololosescreen.h"

#include <vector>

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/metglobalsettingssaverscreen.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "egb";
static const char *const kDirectory = "metagame/_Solo";
static const char *const kContainerName = "end_game_butts";

// The two buttons EnterAndShow() appends, in ring order, and their prompt keys.
static const char *const kRetryButtonObject = "egb_01.but";
static const char *const kLevelsButtonObject = "egb_02.but";
static const char *const kRetryPrompt = "egsl_again";
static const char *const kLevelsPrompt = "egsl_levels";

// Screens the class pushes and exits by registry key.
static const char *const kOwnScreenName = "MetSoloLoseScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kSoloStatsScreen = "MetSoloStatsScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
static const char *const kSoloStagesScreen = "MetSoloStagesScreen";

// The caption key, and the help layout ShowButtons() selects.
static const char *const kCaptionKey = "solo_lose";
static const char *const kPromptLayout = "no_back_title";

// The empty literal the select path passes to clear the panel and the prompt.
static const char *const kNoName = "";

// Configuration codes the button prompts and the caption are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kCaptionConfigCode = 0x269;

// The value both MetFrontEndState flags must hold for EnterAndShow() to save the settings first.
constexpr int kFrontEndFlagSet = 1;

// The retry button, which ShowButtons() selects and slot 36 tests for.
constexpr int kRetryButtonIndex = 0;

// The selection slot 36 leaves behind, which is none.
constexpr int kNoSelection = -1;

// The mExitChoice value the select command records.
constexpr int kSelectRequested = 2;

// The selection alternation the select path starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// Slot 36 lets the renderer resolve the arena view rather than skipping it.
constexpr int kResolveArenaView = 0;

} // namespace

// 0x00399be8
MetSoloLoseScreen::MetSoloLoseScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(new MetButtonList()) {
}

// 0x0039e010
MetSoloLoseScreen::~MetSoloLoseScreen() {
    delete mButtonList;
}

// 0x0039df88
MetSoloLoseScreen *MetSoloLoseScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSoloLoseScreen(pRenderer, nPriority);
}

// 0x00399f88
void MetSoloLoseScreen::EnterAndShow() {
    mRenderer->SetActivePanel(this);
    SetShowing(0);
    mButtonList->Clear();

    HxStr retryLabel = QueryConfigString(kPromptConfigCode, kRetryPrompt);
    mButtonList->Add(HxStr(kRetryButtonObject), retryLabel);

    HxStr levelsLabel = QueryConfigString(kPromptConfigCode, kLevelsPrompt);
    mButtonList->Add(HxStr(kLevelsButtonObject), levelsLabel);

    mHelpKeys.clear();
    mHelpKeys.push_back(HxStr(kRetryPrompt));
    mHelpKeys.push_back(HxStr(kLevelsPrompt));

    if ((MetFrontEndState::shared()->mUsingMemcard == kFrontEndFlagSet) &&
        (MetFrontEndState::shared()->mSettingsDirty == kFrontEndFlagSet)) {
        MetFrontEndState::shared()->mSettingsDirty = 0;
        std::vector<HxStr> screens(1, HxStr());
        screens[0] = kOwnScreenName;
        MetGlobalSettingsSaverScreen::StartSave(screens);
        mActivatePending = 0;
    } else {
        ShowButtons();
    }
}

// 0x00399db8
void MetSoloLoseScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtonList->SelectPrevious();
        MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandNext:
        mButtonList->SelectNext();
        MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        mExitChoice = kSelectRequested;
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kSelectAlternateInterval,
                            mButtonList->mSelectedButton,
                            kSelectAlternateCycles);
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        break;

    default:
        break;
    }
}

// 0x0039df80
void MetSoloLoseScreen::PlayLeaveSound([[maybe_unused]] int nSelector) {
}

// 0x0039df70
void MetSoloLoseScreen::PlayCycleLeftSound([[maybe_unused]] int nSelector) {
}

// 0x0039df78
void MetSoloLoseScreen::PlayCycleRightSound([[maybe_unused]] int nSelector) {
}

// 0x0039a6e8
void MetSoloLoseScreen::OnRepeatingSoundFinished([[maybe_unused]] Rnd::Button *pButton) {
    ExitScreenByName(HxStr(kSoloStatsScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}

// 0x0039a880
void MetSoloLoseScreen::OnExitFinished() {
    if ((mButtonList->mSelected == kRetryButtonIndex) && (mExitChoice != 0)) {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kOwnScreenName);
        PushNamedScreen(HxStr(kLoadGameScreen));
        ActivateNamedPanel(HxStr(kLoadGameScreen));
    } else {
        mRenderer->ResolveArenaView(kResolveArenaView);
        mRenderer->OnReturnFromGame();
        mRenderer->OnReturnToMenus();
        PushNamedScreen(HxStr(kSoloStagesScreen));
        ActivateNamedPanel(HxStr(kSoloStagesScreen));
    }
    mButtonList->SetSelected(kNoSelection);
}

// 0x0039a4e8
void MetSoloLoseScreen::ShowButtons() {
    {
        HxStr caption = QueryConfigString(kCaptionConfigCode, kCaptionKey);
        MetScreenTitleScreen::SetTitle(caption);
    }
    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));
    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kSoloStatsScreen));
    mRenderer->SetActivePanel(this);
    mButtonList->SetSelected(kRetryButtonIndex);
    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
    MetScreen::EnterAndShow();
}
