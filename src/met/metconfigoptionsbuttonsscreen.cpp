#include "met/metconfigoptionsbuttonsscreen.h"

#include "game/globalsettings.h"
#include "met/gameoptions.h"
#include "met/metconfigcontrollerscreen.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/view.h"

namespace {

static const char *const kScreenName = "nob";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "net_options_butts";

constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// MetScreen::mExitChoice on exit, read back by OnExitFinished().
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

} // namespace

// NTSC-U/C: 0x002071f0, PAL: 0x00210008
MetConfigOptionsButtonsScreen::MetConfigOptionsButtonsScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mOptionButtons(nullptr), mControllerIndex(kFirstController) {
    mOptionButtons = new MetButtonList();
}

// NTSC-U/C: 0x002073c8, PAL: 0x00210240
void MetConfigOptionsButtonsScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mOptionButtons->SelectPrevious();
        MetHelpScreen::SetText(mHelpKeys[mOptionButtons->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandNext:
        mOptionButtons->SelectNext();
        MetHelpScreen::SetText(mHelpKeys[mOptionButtons->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandSelect:
        if (mOptionButtons->mSelectedButton->mName == kControllerButton) {
            mControllerIndex = pCommand->mPadIndex - 1;
        }
        ActivateNamedPanel(HxStr(""));
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kButtonFlashInterval,
                            mOptionButtons->mSelectedButton,
                            kButtonFlashCycles);
        break;

    case kMetScreenCommandBack:
        mExitChoice = kExitCancelled;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kRightGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x00207660, PAL: 0x00210540
void MetConfigOptionsButtonsScreen::EnterAndShow() {
    const int bDiscButton = GlobalSettings::shared()->mGameOptions.mExpansionPack;

    // Yes, the binary does not test either frame view for null.
    dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kFiveButtonFrame)))
        ->SetShowing(bDiscButton);
    dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr(kFourButtonFrame)))
        ->SetShowing(bDiscButton ^ 1);

    mOptionButtons->Clear();
    mHelpKeys.clear();

    mOptionButtons->Add(HxStr(kGameButton),
                        MetConfigText(kMetStrNobGameSetup, kPromptConfigCode, kGamePrompt));
    mOptionButtons->Add(
        HxStr(kControllerButton),
        MetConfigText(kMetStrNobControllerSetup, kPromptConfigCode, kControllerPrompt));
    mOptionButtons->Add(HxStr(kMemoryButton),
                        MetConfigText(kMetStrNobMemCardSetup, kPromptConfigCode, kMemoryPrompt));
    mOptionButtons->Add(HxStr(kCreditsButton),
                        MetConfigText(kMetStrNobCredits, kPromptConfigCode, kCreditsPrompt));
    if (bDiscButton) {
        mOptionButtons->Add(HxStr(kDiscButton),
                            MetConfigText(kMetStrNobDiskChange, kPromptConfigCode, kDiscPrompt));
    }

    // The European release stores the help texts themselves rather than their keys.
    mHelpKeys.push_back(MetText(kMetStrHNobGameSetup, kGamePrompt));
    mHelpKeys.push_back(MetText(kMetStrHNobControllerSetup, kControllerPrompt));
    mHelpKeys.push_back(MetText(kMetStrHNobMemCardSetup, kMemoryPrompt));
    mHelpKeys.push_back(MetText(kMetStrHNobCredits, kCreditsPrompt));
    if (bDiscButton) {
        mHelpKeys.push_back(MetText(kMetStrHNobDiskChange, kDiscPrompt));
    }

    mOptionButtons->SetSelected(kFirstButton);
    mControllerIndex = kFirstController;
    MetScreenTitleScreen::SetTitle(
        MetConfigText(kMetStrTConfigOptionButtons, kTitleConfigCode, kTitleKey));
    MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardPreset));
    MetHelpScreen::SetText(mHelpKeys[mOptionButtons->mSelected], mRenderer->mAnimationFrame);
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x00207fc0, PAL: 0x00211070
void MetConfigOptionsButtonsScreen::OnRepeatingSoundFinished(Rnd::Button *pButton) {
    HxStr name(pButton->mName);
    if (name == kControllerButton || name == kMemoryButton || name == kGameButton ||
        name == kCreditsButton || name == kDiscButton) {
        mExitChoice = kExitToButton;
        ExitScreenByName(HxStr(kRightGizmoScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        if (name == kCreditsButton || name == kDiscButton || name == kMemoryButton) {
            ExitScreenByName(HxStr(kHelpScreen));
        }
    }
}

// NTSC-U/C: 0x00208270, PAL: 0x00211398
void MetConfigOptionsButtonsScreen::OnExitFinished() {
    if (mExitChoice == kExitCancelled) {
        if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen) {
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

    HxStr name(mOptionButtons->mSelectedButton->mName);
    if (name == kControllerButton) {
        // Yes, the binary does not test the cast for null.
        dynamic_cast<MetConfigControllerScreen *>(FindScreenByName(HxStr(kControllerScreen)))
            ->mControllerIndex = mControllerIndex;
        PushNamedScreen(HxStr(kControllerScreen));
        ActivateNamedPanel(HxStr(kControllerScreen));
    } else if (name == kMemoryButton) {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kThisScreen);
#ifdef VIDEO_STANDARD_PAL
        // Activating first ends with the focus on the warning EnterAndShow() opens.
        ActivateNamedPanel(HxStr(kMemCardLoadScreen));
        PushNamedScreen(HxStr(kMemCardLoadScreen));
#else
        PushNamedScreen(HxStr(kMemCardLoadScreen));
        ActivateNamedPanel(HxStr(kMemCardLoadScreen));
#endif
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

// NTSC-U/C: 0x0020bff0, PAL: 0x00215420
void MetConfigOptionsButtonsScreen::PlayCycleLeftSound([[maybe_unused]] int nSelector) {
}

// NTSC-U/C: 0x0020bff8, PAL: 0x00215428
void MetConfigOptionsButtonsScreen::PlayCycleRightSound([[maybe_unused]] int nSelector) {
}

// NTSC-U/C: 0x0020c000, PAL: 0x00215430
MetConfigOptionsButtonsScreen *MetConfigOptionsButtonsScreen::New(MetRenderer *pRenderer,
                                                                  int nPriority) {
    return new MetConfigOptionsButtonsScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x0020c088, PAL: 0x002154b8
MetConfigOptionsButtonsScreen::~MetConfigOptionsButtonsScreen() {
    delete mOptionButtons;
}
