#include "met/metconfigoptionsbuttonsscreen.h"

#include "game/globalsettings.h"
#include "met/gameoptions.h"
#include "met/metconfigcontrollerscreen.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/view.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "nob";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "net_options_butts";

constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// MetScreen::mUnknown18 on exit, read back by OnUnknownSlot36().
constexpr int kExitCancelled = 0;
constexpr int kExitToButton = 2;

constexpr int kFirstButton = 0;
constexpr int kFirstController = 0;
constexpr int kButtonFlashCycles = 2;
constexpr float kButtonFlashInterval = 30.0f;

static const char *const kFiveButtonFrame = "nob_5group.view";
static const char *const kFourButtonFrame = "nob_4group.view";

static const char *const kGameButton = "nob_game.but";
static const char *const kControllerButton = "nob_controller.but";
static const char *const kMemoryButton = "nob_memory.but";
static const char *const kCreditsButton = "nob_credits.but";
static const char *const kDiscButton = "nob_disc.but";

static const char *const kGamePrompt = "nob_game_setup";
static const char *const kControllerPrompt = "nob_controller_setup";
static const char *const kMemoryPrompt = "nob_mem_card_setup";
static const char *const kCreditsPrompt = "nob_credits";
static const char *const kDiscPrompt = "nob_disk_change";

static const char *const kTitleKey = "config_option_buttons";
static const char *const kStandardPreset = "standard_title";

static const char *const kPauseGameScreen = "MetPauseSoloGameScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kRightGizmoScreen = "MetRightGizmoScreen";
static const char *const kTopLogoScreen = "MetTopLogoScreen";
static const char *const kLeftGizmoSmallScreen = "MetLeftGizmoSmallScreen";
static const char *const kMainScreen = "MetMainScreen";
static const char *const kControllerScreen = "MetConfigControllerScreen";
static const char *const kThisScreen = "MetConfigOptionsButtonsScreen";
static const char *const kMemCardLoadScreen = "MetMemCardLoadScreen";
static const char *const kGameOptionsScreen = "MetConfigGameOptionsScreen";
static const char *const kCreditsScreen = "MetCreditsScreen";
static const char *const kExpansionPakScreen = "MetExpansionPakScreen";

inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr text;
    QueryConfigString(&text, nCode, pszKey);
    return text;
}

} // namespace

// 0x002071f0
MetConfigOptionsButtonsScreen::MetConfigOptionsButtonsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown8c(nullptr), mUnknown90(kFirstController) {
    mUnknown8c = new MetButtonList();
}

// 0x002073c8
void MetConfigOptionsButtonsScreen::HandleCommand(const MetScreenCommand *pCommand) {
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
        if (mUnknown8c->mUnknown00->mName == kControllerButton) {
            mUnknown90 = pCommand->mPadIndex - 1;
        }
        ActivateNamedPanel(HxStr(""));
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kButtonFlashInterval,
                            mUnknown8c->mUnknown00,
                            kButtonFlashCycles);
        break;

    case kMetScreenCommandBack:
        mUnknown18 = kExitCancelled;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kRightGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x00207660
void MetConfigOptionsButtonsScreen::EnterAndShow() {
    const int bDiscButton = GlobalSettings::shared()->mGameOptions.mUnknown04;

    // Yes, the binary does not test either frame view for null.
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kFiveButtonFrame)))
        ->SetShowing(bDiscButton);
    dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr(kFourButtonFrame)))
        ->SetShowing(bDiscButton ^ 1);

    mUnknown8c->Clear();
    mUnknown38.clear();

    mUnknown8c->Add(HxStr(kGameButton), ConfigText(kPromptConfigCode, kGamePrompt));
    mUnknown8c->Add(HxStr(kControllerButton), ConfigText(kPromptConfigCode, kControllerPrompt));
    mUnknown8c->Add(HxStr(kMemoryButton), ConfigText(kPromptConfigCode, kMemoryPrompt));
    mUnknown8c->Add(HxStr(kCreditsButton), ConfigText(kPromptConfigCode, kCreditsPrompt));
    if (bDiscButton) {
        mUnknown8c->Add(HxStr(kDiscButton), ConfigText(kPromptConfigCode, kDiscPrompt));
    }

    mUnknown38.push_back(HxStr(kGamePrompt));
    mUnknown38.push_back(HxStr(kControllerPrompt));
    mUnknown38.push_back(HxStr(kMemoryPrompt));
    mUnknown38.push_back(HxStr(kCreditsPrompt));
    if (bDiscButton) {
        mUnknown38.push_back(HxStr(kDiscPrompt));
    }

    mUnknown8c->SetSelected(kFirstButton);
    mUnknown90 = kFirstController;
    MetScreenTitleScreen::SetTitle(ConfigText(kTitleConfigCode, kTitleKey));
    MetHelpScreen::SelectPreset(HxStr(kStandardPreset));
    MetHelpScreen::SetText(mUnknown38[mUnknown8c->mSelected], mUnknown10->mUnknown68);
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MetScreen::EnterAndShow();
}

// 0x00207fc0
void MetConfigOptionsButtonsScreen::OnUnknownSlot30(Rnd::Button *pButton) {
    HxStr name(pButton->mName);
    if (name == kControllerButton || name == kMemoryButton || name == kGameButton ||
        name == kCreditsButton || name == kDiscButton) {
        mUnknown18 = kExitToButton;
        ExitScreenByName(HxStr(kRightGizmoScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        if (name == kCreditsButton || name == kDiscButton || name == kMemoryButton) {
            ExitScreenByName(HxStr(kHelpScreen));
        }
    }
}

// 0x00208270
void MetConfigOptionsButtonsScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitCancelled) {
        if (MetFrontEndState::shared()->mUnknown24 == kPauseGameScreen) {
            ExitScreenByName(HxStr(kHelpScreen));
            PushNamedScreen(HxStr(kPauseGameScreen));
            ActivateNamedPanel(HxStr(kPauseGameScreen));
        } else {
            PushNamedScreen(HxStr(kTopLogoScreen));
            PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
            PushNamedScreen(HxStr(kMainScreen));
            ActivateNamedPanel(HxStr(kMainScreen));
        }
        return;
    }

    HxStr name(mUnknown8c->mUnknown00->mName);
    if (name == kControllerButton) {
        // Yes, the binary does not test the cast for null.
        dynamic_cast<MetConfigControllerScreen *>(FindScreenByName(HxStr(kControllerScreen)))
            ->mUnknownc8 = mUnknown90;
        PushNamedScreen(HxStr(kControllerScreen));
        ActivateNamedPanel(HxStr(kControllerScreen));
    } else if (name == kMemoryButton) {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kThisScreen);
        PushNamedScreen(HxStr(kMemCardLoadScreen));
        ActivateNamedPanel(HxStr(kMemCardLoadScreen));
    } else if (name == kGameButton) {
        PushNamedScreen(HxStr(kGameOptionsScreen));
        ActivateNamedPanel(HxStr(kGameOptionsScreen));
    } else if (name == kCreditsButton) {
        PushNamedScreen(HxStr(kCreditsScreen));
        ActivateNamedPanel(HxStr(kCreditsScreen));
    } else if (name == kDiscButton) {
        PushNamedScreen(HxStr(kExpansionPakScreen));
    } else {
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
    }
}

// 0x0020bff0
void MetConfigOptionsButtonsScreen::PlayCycleLeftSound([[maybe_unused]] int nSelector) {
}

// 0x0020bff8
void MetConfigOptionsButtonsScreen::PlayCycleRightSound([[maybe_unused]] int nSelector) {
}

// 0x0020c000
MetConfigOptionsButtonsScreen *MetConfigOptionsButtonsScreen::New(MetRenderer *pRenderer,
                                                                  int nPriority) {
    return new MetConfigOptionsButtonsScreen(pRenderer, nPriority);
}

// 0x0020c088
MetConfigOptionsButtonsScreen::~MetConfigOptionsButtonsScreen() {
    delete mUnknown8c;
}
