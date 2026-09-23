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
#include "os/hxstr.h"
#include "script/configquery.h"

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

// What MetScreen::mUnknown18 records for slot 36 to act on.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The alternation the select command starts on the chosen button.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// MetRenderer::mUnknownd4 on the menu, and while the solo game is chosen.
constexpr int kMenuHighestPad = 4;
constexpr int kSoloHighestPad = 1;

// The one return screen the settings save receives.
constexpr int kSaveReturnScreenCount = 1;

// Add one button labelled from configuration. Slot 38 expands it for each button.
inline void AddButton(MetButtonList *pList, const char *pszObjectName, const char *pszLabelKey) {
    HxStr objectName(pszObjectName);
    HxStr label;
    QueryConfigString(&label, kLabelConfigCode, pszLabelKey);
    pList->Add(objectName, label);
}

} // namespace

// 0x002c60d0
MetMainScreen::MetMainScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr) {
    mButtonList = new MetButtonList;
}

// 0x002cb570
MetMainScreen::~MetMainScreen() {
    delete mButtonList;
}

// 0x002cb4e8
MetMainScreen *MetMainScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMainScreen(pRenderer, nPriority);
}

// 0x002c6dc0
void MetMainScreen::EnterAndShow() {
    mUnknown10->mUnknownd4 = kMenuHighestPad;
    SetShowing(0);
    bool bSettingsChanged = false;
    if (MetFrontEndState::shared()->mUnknown18 != 0) {
        bSettingsChanged = MetFrontEndState::shared()->mUnknown10 != 0;
    }
    if (!bSettingsChanged) {
        EnterMenu();
        return;
    }
    MetFrontEndState::shared()->mUnknown10 = 0;
    std::vector<HxStr> screens;
    screens.resize(kSaveReturnScreenCount);
    screens[0] = kMainScreen;
    MetGlobalSettingsSaverScreen::StartSave(screens);
    mUnknown50 = 0;
}

// 0x002c7030
void MetMainScreen::EnterMenu() {
    if (MetFrontEndState::shared()->mUnknown18 != 0) {
        int nTransition = MetFrontEndState::shared()->mUnknown18;
        MetFrontEndState::shared()->mUnknown18 = 0;
        MetFrontEndState::shared()->mUnknown1c = nTransition;
        MetFrontEndState::shared()->mUnknown24 = HxStr(kMainScreen);
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        mUnknown10->SetActivePanel(this);
    }
    MetHelpScreen::SelectPreset(HxStr(kStandardTitlePreset));
    MetScreen::EnterAndShow();
}

// 0x002c6820
void MetMainScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtonList->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mButtonList->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kSelectAlternateInterval,
                            mButtonList->mUnknown00,
                            kSelectAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
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

// 0x002cb4d8
void MetMainScreen::PlayCycleLeftSound(int) {
}

// 0x002cb4e0
void MetMainScreen::PlayCycleRightSound(int) {
}

// 0x002c6c20
void MetMainScreen::OnUnknownSlot30(Rnd::Object *) {
    mUnknown18 = kExitToButtonAction;
    ExitScreenByName(HxStr(kTitleScreen));
    ExitScreenByName(HxStr(kLeftGizmoSmallScreen));
    ExitScreenByName(HxStr(kTopLogoScreen));
    BeginExit();
}

// 0x002c72a0
void MetMainScreen::OnUnknownSlot33() {
    if (MetFrontEndState::shared()->mUnknown24 == kLogoScreen ||
        mButtonList->mSelected == kNoButton) {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kNoName);
        if (GlobalSettings::shared()->mTutorialComplete == 0) {
            mButtonList->SetSelected(kTutorialButtonIndex);
        } else {
            mButtonList->SetSelected(kSoloButtonIndex);
        }
    }
    Application::shared()->GetGameManager()->SetGameMode(kGameModeNone);
    MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
}

// 0x002c73e0
void MetMainScreen::OnUnknownSlot36() {
    if (mUnknown18 != kExitBack) {
        OpenSelectedButton();
        return;
    }
    mButtonList->SetSelected(kNoButton);
    PushNamedScreen(HxStr(kLogoScreen));
    ActivateNamedPanel(HxStr(kLogoScreen));
}

// 0x002c7520
void MetMainScreen::OpenSelectedButton() {
    switch (mButtonList->mSelected) {
    case kTutorialButtonIndex:
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kTutorialScreen));
        ActivateNamedPanel(HxStr(kTutorialScreen));
        break;

    case kSoloButtonIndex:
        mUnknown10->OnUnknown00390090();
        Application::shared()->GetGameManager()->SetGameMode(kGameModeSolo);
        mUnknown10->mUnknownd4 = kSoloHighestPad;
        if (MetFrontEndState::shared()->mUnknown0c == 0) {
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
        mUnknown10->OnUnknown00390090();
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

// 0x002c62a0
void MetMainScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    AddButton(mButtonList, kTutorialButton, kTutorialLabel);
    AddButton(mButtonList, kSoloButton, kSoloLabel);
    AddButton(mButtonList, kMultiButton, kMultiLabel);
    AddButton(mButtonList, kOptionsButton, kOptionsLabel);
    mUnknown38.clear();
    mUnknown38.push_back(HxStr(kTutorialHelp));
    mUnknown38.push_back(HxStr(kSoloHelp));
    mUnknown38.push_back(HxStr(kMultiHelp));
    mUnknown38.push_back(HxStr(kOptionsHelp));
}
