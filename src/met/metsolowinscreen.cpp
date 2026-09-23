#include "met/metsolowinscreen.h"

#include "met/metbuttonlist.h"
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

// Index EnterAndShow() selects, which is the continue button.
constexpr int kContinueButtonIndex = 0;

// The selection alternation the select path starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

} // namespace

MetSoloWinScreen::MetSoloWinScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown90(new MetButtonList()), mDifficultyUnlocked(0) {
}

MetSoloWinScreen::~MetSoloWinScreen() {
    delete mUnknown90;
}

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

void MetSoloWinScreen::PlayLeaveSound(int) {
}

void MetSoloWinScreen::PlayCycleLeftSound(int) {
}

void MetSoloWinScreen::PlayCycleRightSound(int) {
}

void MetSoloWinScreen::OnUnknownSlot30(Rnd::Object *) {
    ExitScreenByName(HxStr(kSoloStatsScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    ExitScreenByName(HxStr(kEndGameGizmoScreen));
    BeginExit();
}

void MetSoloWinScreen::OnUnknownSlot33() {
    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));
    MetHelpScreen::SetText(mUnknown38[mUnknown90->mSelected], mUnknown10->mUnknown68);
}

void MetSoloWinScreen::SetDifficultyUnlocked(int nUnlocked) {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    MetSoloWinScreen *pWinScreen =
        pScreen != nullptr ? dynamic_cast<MetSoloWinScreen *>(pScreen) : nullptr;
    pWinScreen->mDifficultyUnlocked = nUnlocked; // The binary does not test the result for null.
}
