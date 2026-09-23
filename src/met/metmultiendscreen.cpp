#include "met/metmultiendscreen.h"

#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
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
      mUnknown8c(nullptr) {
    mUnknown8c = new MetButtonList();
}

// 0x002f5d20
MetMultiEndScreen::~MetMultiEndScreen() {
    delete mUnknown8c;
}

// 0x002f5f90
void MetMultiEndScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown8c->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mUnknown8c->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mUnknown8c->mUnknown00,
                            kSelectAlternateCycles);
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        break;

    default:
        break;
    }
}

// 0x002f6158
void MetMultiEndScreen::EnterAndShow() {
    mUnknown8c->Clear();

    const HxStr againButton(kAgainButtonObject);
    HxStr againLabel;
    QueryConfigString(&againLabel, kPromptConfigCode, kAgainPrompt);
    mUnknown8c->Add(againButton, againLabel);

    const HxStr newButton(kNewButtonObject);
    HxStr newLabel;
    QueryConfigString(&newLabel, kPromptConfigCode, kNewPrompt);
    mUnknown8c->Add(newButton, newLabel);

    mUnknown38.clear();
    mUnknown38.push_back(HxStr(kAgainPrompt));
    mUnknown38.push_back(HxStr(kNewPrompt));

    {
        HxStr title;
        QueryConfigString(&title, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    }
    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));
    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kMultiStatsScreen));
    mUnknown10->SetActivePanel(this);
    mUnknown8c->SetSelected(kAgainButtonIndex);
    MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
    MetScreen::EnterAndShow();
}

// 0x002f6640
void MetMultiEndScreen::OnUnknownSlot30(Rnd::Button *) {
    ExitScreenByName(HxStr(kMultiStatsScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kHelpScreen));
    BeginExit();
}

// 0x002f67d8
void MetMultiEndScreen::OnUnknownSlot36() {
    if (mUnknown8c->mSelected == kAgainButtonIndex) {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kOwnScreenName);
        PushNamedScreen(HxStr(kLoadGameScreen));
        ActivateNamedPanel(HxStr(kLoadGameScreen));
    } else {
        mUnknown10->ResolveArenaView(kResolveArenaView);
        mUnknown10->OnUnknown00390088();
        mUnknown10->OnUnknown00390090();
        PushNamedScreen(HxStr(kSoloStagesScreen));
        ActivateNamedPanel(HxStr(kSoloStagesScreen));
    }
    mUnknown8c->SetSelected(kNoSelection);
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
