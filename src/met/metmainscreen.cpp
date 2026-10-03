#include "met/metmainscreen.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/globalsettings.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/metglobalsettingssaverscreen.h"
#include "met/methelpscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metstrings.h"
#include "os/hxstr.h"

namespace {

static const char *const kScreenName = "fm";
static const char *const kDirectory = "metagame/_Solo";
static const char *const kContainerName = "2_main";

// The four buttons in ring order, the keys their labels are read under, and their help texts.
static const char *const kTutorialButton = "ms_tut.but";
static const char *const kSoloButton = "ms_solo.but";
static const char *const kMultiButton = "ms_multi.but";
static const char *const kOptionsButton = "ms_options.but";
static const char *const kTutorialLabel = "tut";
static const char *const kSoloLabel = "solo";
static const char *const kMultiLabel = "multi";
static const char *const kOptionsLabel = "opt";
static const char *const kTutorialHelp = "ms_tut";
static const char *const kSoloHelp = "ms_solo";
static const char *const kMultiHelp = "ms_multi";
static const char *const kOptionsHelp = "ms_opt";

static const char *const kStandardTitlePreset = "standard_title";

static const char *const kMainScreen = "MetMainScreen";
static const char *const kLogoScreen = "MetLogoScreen";
static const char *const kTopLogoScreen = "MetTopLogoScreen";
static const char *const kLeftGizmoSmallScreen = "MetLeftGizmoSmallScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kRightGizmoScreen = "MetRightGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLoadPreFabScreen = "MetLoadPreFabScreen";
static const char *const kLoadFreqScreen = "MetLoadFreqScreen";
static const char *const kLoadNewFreqScreen = "MetLoadNewFreqScreen";
static const char *const kLocNumPlayersScreen = "MetLocNumPlayersScreen";
static const char *const kTutorialScreen = "MetTutorialScreen";
static const char *const kConfigOptionsButtonsScreen = "MetConfigOptionsButtonsScreen";
static const char *const kNoName = "";

// Configuration code the button labels are read under.
constexpr int kLabelConfigCode = 0x258;

// The button ring indices.
enum MainButton {
    kNoButton = -1,
    kTutorialButtonIndex = 0,
    kSoloButtonIndex = 1,
    kMultiButtonIndex = 2,
    kOptionsButtonIndex = 3
};

// What MetScreen::mExitChoice records for slot 36 to act on.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The alternation the select command starts on the chosen button.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// MetRenderer::mMaxPadIndex on the menu, and while the solo game is chosen.
constexpr int kMenuHighestPad = 4;
constexpr int kSoloHighestPad = 1;

// The one return screen the settings save receives.
constexpr int kSaveReturnScreenCount = 1;

// Add one button labelled from configuration. Slot 38 expands it for each button.
inline void AddButton(MetButtonList *pList,
                      const char *pszObjectName,
                      MetStringId nLabelId,
                      const char *pszLabelKey) {
    HxStr objectName(pszObjectName);
    HxStr label = MetConfigText(nLabelId, kLabelConfigCode, pszLabelKey);
    pList->Add(objectName, label);
}

} // namespace

// NTSC-U/C: 0x002c60d0, PAL: 0x002e6898
MetMainScreen::MetMainScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr) {
    mButtonList = new MetButtonList;
}

// NTSC-U/C: 0x002cb570, PAL: 0x002ec458
MetMainScreen::~MetMainScreen() {
    delete mButtonList;
}

// NTSC-U/C: 0x002cb4e8, PAL: 0x002ec3d0
MetMainScreen *MetMainScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMainScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x002c6dc0, PAL: 0x002e7850
void MetMainScreen::EnterAndShow() {
    mRenderer->mMaxPadIndex = kMenuHighestPad;
    SetShowing(0);
    bool bSettingsChanged = false;
    if (MetFrontEndState::shared()->mPendingTransition != 0) {
        bSettingsChanged = MetFrontEndState::shared()->mSettingsDirty != 0;
    }
    if (!bSettingsChanged) {
        EnterMenu();
        return;
    }
    MetFrontEndState::shared()->mSettingsDirty = 0;
    std::vector<HxStr> screens;
    screens.resize(kSaveReturnScreenCount);
    screens[0] = kMainScreen;
    MetGlobalSettingsSaverScreen::StartSave(screens);
    mActivatePending = 0;
}

// NTSC-U/C: 0x002c7030, PAL: 0x002e7b08
void MetMainScreen::EnterMenu() {
    if (MetFrontEndState::shared()->mPendingTransition != 0) {
        int nTransition = MetFrontEndState::shared()->mPendingTransition;
        MetFrontEndState::shared()->mPendingTransition = 0;
        MetFrontEndState::shared()->mLastTransition = nTransition;
        MetFrontEndState::shared()->mReturnScreen = HxStr(kMainScreen);
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        mRenderer->SetActivePanel(this);
    }
    MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardTitlePreset));
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x002c6820, PAL: 0x002e7180
void MetMainScreen::HandleCommand(const MetScreenCommand *pCommand) {
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
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kSelectAlternateInterval,
                            mButtonList->mSelectedButton,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTopLogoScreen));
        ExitScreenByName(HxStr(kLeftGizmoSmallScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x002cb4d8, PAL: 0x002ec3c0
void MetMainScreen::PlayCycleLeftSound(int) {
}

// NTSC-U/C: 0x002cb4e0, PAL: 0x002ec3c8
void MetMainScreen::PlayCycleRightSound(int) {
}

// NTSC-U/C: 0x002c6c20, PAL: 0x002e7650
void MetMainScreen::OnRepeatingSoundFinished(Rnd::Button *) {
    mExitChoice = kExitToButtonAction;
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kLeftGizmoSmallScreen));
    ExitScreenByName(HxStr(kTopLogoScreen));
    BeginExit();
}

// NTSC-U/C: 0x002c72a0, PAL: 0x002e7e10
void MetMainScreen::OnEnterFinished() {
    if (MetFrontEndState::shared()->mReturnScreen == kLogoScreen ||
        mButtonList->mSelected == kNoButton) {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kNoName);
        if (GlobalSettings::shared()->mTutorialComplete == 0) {
            mButtonList->SetSelected(kTutorialButtonIndex);
        } else {
            mButtonList->SetSelected(kSoloButtonIndex);
        }
    }
    Application::shared()->GetGameManager()->SetGameMode(kGameModeNone);
    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
#ifdef VIDEO_STANDARD_PAL
    ActivateNamedPanel(HxStr(kMainScreen));
#endif
}

// NTSC-U/C: 0x002c73e0, PAL: 0x002e8000
void MetMainScreen::OnExitFinished() {
    if (mExitChoice != kExitBack) {
        OpenSelectedButton();
        return;
    }
    mButtonList->SetSelected(kNoButton);
    PushNamedScreen(HxStr(kLogoScreen));
    ActivateNamedPanel(HxStr(kLogoScreen));
}

// NTSC-U/C: 0x002c7520, PAL: 0x002e8180
void MetMainScreen::OpenSelectedButton() {
    switch (mButtonList->mSelected) {
    case kTutorialButtonIndex:
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kTutorialScreen));
        ActivateNamedPanel(HxStr(kTutorialScreen));
        break;

    case kSoloButtonIndex:
        mRenderer->OnReturnToMenus();
        Application::shared()->GetGameManager()->SetGameMode(kGameModeSolo);
        mRenderer->mMaxPadIndex = kSoloHighestPad;
        if (MetFrontEndState::shared()->mUsingMemcard == 0) {
            PushNamedScreen(HxStr(kLoadPreFabScreen));
            ActivateNamedPanel(HxStr(kLoadPreFabScreen));
        } else if (MetPersonaData::loadList()->size() != 0) {
            PushNamedScreen(HxStr(kLoadFreqScreen));
            ActivateNamedPanel(HxStr(kLoadFreqScreen));
        } else {
            PushNamedScreen(HxStr(kLoadNewFreqScreen));
            ActivateNamedPanel(HxStr(kLoadNewFreqScreen));
        }
        break;

    case kMultiButtonIndex:
        mRenderer->OnReturnToMenus();
        Application::shared()->GetGameManager()->SetGameMode(kGameModeLocal);
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kLocNumPlayersScreen));
        ActivateNamedPanel(HxStr(kLocNumPlayersScreen));
        break;

    case kOptionsButtonIndex:
        PushNamedScreen(HxStr(kRightGizmoScreen));
        PushNamedScreen(HxStr(kConfigOptionsButtonsScreen));
        ActivateNamedPanel(HxStr(kConfigOptionsButtonsScreen));
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x002c62a0, PAL: 0x002e6ad0
void MetMainScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    AddButton(mButtonList, kTutorialButton, kMetStrTut, kTutorialLabel);
    AddButton(mButtonList, kSoloButton, kMetStrSolo, kSoloLabel);
    AddButton(mButtonList, kMultiButton, kMetStrMulti, kMultiLabel);
    AddButton(mButtonList, kOptionsButton, kMetStrOpt, kOptionsLabel);
    mHelpKeys.clear();
    // The European release stores the help texts themselves rather than their keys.
    mHelpKeys.push_back(MetText(kMetStrHMsTut, kTutorialHelp));
    mHelpKeys.push_back(MetText(kMetStrHMsSolo, kSoloHelp));
    mHelpKeys.push_back(MetText(kMetStrHMsMulti, kMultiHelp));
    mHelpKeys.push_back(MetText(kMetStrHMsOpt, kOptionsHelp));
}
