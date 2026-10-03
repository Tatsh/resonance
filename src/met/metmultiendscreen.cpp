#include "met/metmultiendscreen.h"

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metstrings.h"
#include "os/hxstr.h"

namespace {

// The screen name, the directory the container loads from, and the container name.
static const char *const kScreenName = "egb";
static const char *const kDirectory = "metagame/_Solo";
static const char *const kContainerName = "end_game_butts";

// The two buttons EnterAndShow() appends, in ring order, and their prompt keys.
static const char *const kAgainButtonObject = "egb_01.but";
static const char *const kNewButtonObject = "egb_02.but";
static const char *const kAgainPrompt = "egmg_again";
static const char *const kNewPrompt = "egmg_new";

// Screens the class pushes and exits by registry key.
static const char *const kOwnScreenName = "MetMultiEndScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kMultiStatsScreen = "MetMultiStatsScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
static const char *const kSoloStagesScreen = "MetSoloStagesScreen";

// The title key, and the help layout EnterAndShow() selects.
static const char *const kTitleKey = "multi_game_over";
static const char *const kPromptLayout = "no_back_title";

// The empty literal the select path passes to clear the panel and the prompt.
static const char *const kNoName = "";

// Configuration codes the button prompts and the title are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The again button, which EnterAndShow() selects and slot 36 tests for.
constexpr int kAgainButtonIndex = 0;

// The selection slot 36 leaves behind, which is none.
constexpr int kNoSelection = -1;

// The selection alternation the select path starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// Slot 36 lets the renderer resolve the arena view rather than skipping it.
constexpr int kResolveArenaView = 0;

} // namespace

// 0x002f58d0
MetMultiEndScreen::MetMultiEndScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr) {
    mButtonList = new MetButtonList();
}

// 0x002f5d20
MetMultiEndScreen::~MetMultiEndScreen() {
    delete mButtonList;
}

// 0x002f5f90
void MetMultiEndScreen::HandleCommand(const MetScreenCommand *pCommand) {
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

// NTSC-U/C: 0x002f6158, PAL: 0x00319fd8
void MetMultiEndScreen::EnterAndShow() {
    mButtonList->Clear();

    const HxStr againButton(kAgainButtonObject);
    HxStr againLabel = MetConfigText(kMetStrEgmgAgain, kPromptConfigCode, kAgainPrompt);
    mButtonList->Add(againButton, againLabel);

    const HxStr newButton(kNewButtonObject);
    HxStr newLabel = MetConfigText(kMetStrEgmgNew, kPromptConfigCode, kNewPrompt);
    mButtonList->Add(newButton, newLabel);

    mHelpKeys.clear();
    // The European release stores the help texts themselves rather than their keys.
    mHelpKeys.push_back(MetText(kMetStrHEgmgAgain, kAgainPrompt));
    mHelpKeys.push_back(MetText(kMetStrHEgmgNew, kNewPrompt));

    {
        HxStr title = MetConfigText(kMetStrTMultiGameOver, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    }
    MetHelpScreen::SelectPreset(MetText(kMetStrHNoBackTitle, kPromptLayout));
    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kMultiStatsScreen));
    mRenderer->SetActivePanel(this);
    mButtonList->SetSelected(kAgainButtonIndex);
    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
    MetScreen::EnterAndShow();
}

// 0x002f6640
void MetMultiEndScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    ExitScreenByName(HxStr(kMultiStatsScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}

// 0x002f67d8
void MetMultiEndScreen::OnExitFinished() {
    if (mButtonList->mSelected == kAgainButtonIndex) {
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

// 0x002f9d98
void MetMultiEndScreen::PlayCycleLeftSound(int) {
}

// 0x002f9da0
void MetMultiEndScreen::PlayCycleRightSound(int) {
}

// 0x002f9da8
void MetMultiEndScreen::PlayLeaveSound(int) {
}

// 0x002f9db0
MetMultiEndScreen *MetMultiEndScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMultiEndScreen(pRenderer, nPriority);
}
