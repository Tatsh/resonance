#include "met/metsolowinscreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "egwb";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "end_win_butts";

// The three buttons EnterAndShow() appends, in ring order.
static const char *const kContinueButtonObject = "egwb_01.but";
static const char *const kExitButtonObject = "egwb_02.but";
static const char *const kRestartButtonObject = "egwb_03.but";

// The three prompts, matching the button order above. Each one is both the key the button's label
// is looked up under and the prompt the help screen displays for that button.
static const char *const kContinuePrompt = "egsw_continue";
static const char *const kExitPrompt = "egsw_exit";
static const char *const kRestartPrompt = "egsw_restart";

// Screens the class pushes and exits by registry key.
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kSoloStatsScreen = "MetSoloStatsScreen";
static const char *const kEndGameGizmoScreen = "MetEndGameGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";

// This screen's own registry key, which SetDifficultyUnlocked() resolves.
static const char *const kOwnScreenName = "MetSoloWinScreen";

// The prompt layout OnUnknownSlot33() selects.
static const char *const kPromptLayout = "no_back_title";

// The key the screen title is looked up under.
static const char *const kTitleKey = "solo_win";

// The empty literal the select path passes to clear the panel and the prompt.
static const char *const kNoName = "";

// Configuration codes the button prompts and the title are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// Index EnterAndShow() selects, which is the continue button, and the exit button after it.
constexpr int kContinueButtonIndex = 0;
constexpr int kExitButtonIndex = 1;

// The screens slot 36 goes on to for each selection.
static const char *const kSoloStagesScreen = "MetSoloStagesScreen";
static const char *const kMainScreen = "MetMainScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";

// The selection slot 36 leaves behind, which is none.
constexpr int kNoSelection = -1;

// Slot 36 lets the renderer resolve the arena view rather than skipping it.
constexpr int kResolveArenaView = 0;

// The selection alternation the select path starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

// 0x003b55a8
MetSoloWinScreen::MetSoloWinScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown90(new MetButtonList()), mDifficultyUnlocked(0) {
}

// 0x003b9ce8
MetSoloWinScreen::~MetSoloWinScreen() {
    delete mUnknown90;
}

// 0x003b9b98
MetSoloWinScreen *MetSoloWinScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSoloWinScreen(pRenderer, nPriority);
}

// 0x003b6188
void MetSoloWinScreen::OnUnknownSlot36() {
    mUnknown10->ResolveArenaView(kResolveArenaView);
    mUnknown10->OnUnknown00390088();
    mUnknown10->OnUnknown00390090();
    if (mDifficultyUnlocked != 0) {
        mDifficultyUnlocked = 0;
        GameParams params(*Application::shared()->GetGameManager()->GetParams());
        ++params.mDifficulty;
        Application::shared()->GetGameManager()->SetParams(params);
    }
    switch (mUnknown90->mSelected) {
    case kContinueButtonIndex:
        PushNamedScreen(HxStr(kSoloStagesScreen));
        ActivateNamedPanel(HxStr(kSoloStagesScreen));
        break;

    case kExitButtonIndex:
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
        break;

    default:
        MetFrontEndState::shared()->mUnknown24 = HxStr(kOwnScreenName);
        PushNamedScreen(HxStr(kLoadGameScreen));
        ActivateNamedPanel(HxStr(kLoadGameScreen));
        break;
    }
    mUnknown90->SetSelected(kNoSelection);
}

// 0x003b5968
void MetSoloWinScreen::EnterAndShow() {
    mUnknown90->Clear();

    HxStr continueLabel;
    QueryConfigString(&continueLabel, kPromptConfigCode, kContinuePrompt);
    mUnknown90->Add(HxStr(kContinueButtonObject), continueLabel);

    HxStr exitLabel;
    QueryConfigString(&exitLabel, kPromptConfigCode, kExitPrompt);
    mUnknown90->Add(HxStr(kExitButtonObject), exitLabel);

    HxStr restartLabel;
    QueryConfigString(&restartLabel, kPromptConfigCode, kRestartPrompt);
    mUnknown90->Add(HxStr(kRestartButtonObject), restartLabel);

    mUnknown90->SetSelected(kContinueButtonIndex);

    mUnknown38.clear();
    mUnknown38.push_back(HxStr(kContinuePrompt));
    mUnknown38.push_back(HxStr(kExitPrompt));
    mUnknown38.push_back(HxStr(kRestartPrompt));

    HxStr title;
    QueryConfigString(&title, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kSoloStatsScreen));
    PushNamedScreen(HxStr(kEndGameGizmoScreen));

    MetScreen::EnterAndShow();
}

// 0x003b57a0
void MetSoloWinScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown90->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mUnknown90->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mUnknown90->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mUnknown90->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mUnknown90->mUnknown00,
                            kSelectAlternateCycles);
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        break;

    default:
        break;
    }
}

// 0x003b9b90
void MetSoloWinScreen::PlayLeaveSound(int) {
}

// 0x003b9b80
void MetSoloWinScreen::PlayCycleLeftSound(int) {
}

// 0x003b9b88
void MetSoloWinScreen::PlayCycleRightSound(int) {
}

// 0x003b5f88
void MetSoloWinScreen::OnUnknownSlot30(Rnd::Button *) {
    ExitScreenByName(HxStr(kSoloStatsScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    ExitScreenByName(HxStr(kEndGameGizmoScreen));
    BeginExit();
}

// 0x003b9d80
void MetSoloWinScreen::OnUnknownSlot33() {
    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));
    MetHelpScreen::SetText(mUnknown38[mUnknown90->mSelected], mUnknown10->mUnknown68);
}

// 0x003b9c20
void MetSoloWinScreen::SetDifficultyUnlocked(int nUnlocked) {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    MetSoloWinScreen *pWinScreen =
        pScreen != nullptr ? dynamic_cast<MetSoloWinScreen *>(pScreen) : nullptr;
    pWinScreen->mDifficultyUnlocked = nUnlocked; // The binary does not test the result for null.
}
