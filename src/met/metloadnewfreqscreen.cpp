#include "met/metloadnewfreqscreen.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/globalsettings.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakerbuttonsscreen.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metkeyboardrequest.h"
#include "met/metkeyboardscreen.h"
#include "met/metpersonadata.h"
#include "met/metpersonasaverscreen.h"
#include "met/metscreentitlescreen.h"
#include "os/datetime.h"
#include "os/hxstr.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

// The three buttons BuildButtonList() appends, in ring order.
static const char *const kNameButtonObject = "cid_01.but";
static const char *const kEditButtonObject = "cid_02.but";
static const char *const kCreateButtonObject = "cid_03.but";

// The three prompts BuildButtonList() appends, matching the button order above.
static const char *const kNamePrompt = "id_name";
static const char *const kEditPrompt = "cid_edit";
static const char *const kCreatePrompt = "id_create";

// Configuration code every label and the title come from, with the key each one passes.
constexpr int kLabelConfigCode = 600;
constexpr int kTitleConfigCode = 617;
static const char *const kNameLabelKey = "nf_enter";
static const char *const kEditLabelKey = "nf_edit";
static const char *const kCreateLabelKey = "nf_create";
static const char *const kTitleKey = "create_char";

// The prompt layout EnterAndShow() selects.
static const char *const kPromptLayout = "standard_title";

// Screens the class pushes, exits, and activates by registry key.
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLoadNewFreqScreen = "MetLoadNewFreqScreen";
static const char *const kFreqMakerCanvasScreen = "MetFreqMakerCanvasScreen";
static const char *const kFreqMakerButtonsScreen = "MetFreqMakerButtonsScreen";

// The MetFreqMakerButtonsScreen::SetEditing() value, and the LoadPrefab() randomise flag.
constexpr int kFreqMakerEditing = 1;
constexpr int kNoRandomize = 0;

// Ring index of the button BuildButtonList() selects and UpdateNameLabel() labels.
constexpr int kNameButtonIndex = 0;

// The keyboard request OnNameButton() builds for the new identity's name.
static const char *const kKeyboardPrompt = "FreQ name";
static const char *const kKeyboardInitialText = "";
static const char *const kKeyboardTicker = "name_new_freq_ticker";
constexpr int kKeyboardMaxWidth = 176;
constexpr int kKeyboardMaxLength = 12;
constexpr int kAnyPad = -1;

// The screens OnKeyboardTextEntered() hands to the persona saver to return to, and their order.
static const char *const kNetPortalScreen = "MetNetPortalScreen";
static const char *const kModeScreen = "MetModeScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
constexpr int kSaveReturnScreenCount = 3;
constexpr int kSaveReturnFirst = 0;
constexpr int kSaveReturnGizmo = 1;
constexpr int kSaveReturnHelp = 2;
// The two values OnKeyboardTextEntered() passes MetPersonaSaverScreen::StartSave() last.
constexpr int kSaveConfirmReplace = 1;
constexpr int kSaveIsCopy = 0;

// The dialogue OnMsgScreenDismissed() answers, which reports a rejected name.
static const char *const kNameRejectedDialogue = "namenogood";

} // namespace

// 0x002a8418
MetLoadNewFreqScreen::MetLoadNewFreqScreen(MetRenderer *pRenderer, int nPriority)
    : MetLoadFreqBaseScreen(pRenderer, nPriority) {
}

// 0x002a8458
MetLoadNewFreqScreen::~MetLoadNewFreqScreen() {
}

// 0x002a8390
MetScreen *MetLoadNewFreqScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLoadNewFreqScreen(pRenderer, nPriority);
}

// 0x002a4108
void MetLoadNewFreqScreen::OnNameButton() {
    MetKeyboardRequest request(HxStr(kLoadNewFreqScreen),
                               HxStr(kKeyboardPrompt),
                               HxStr(kKeyboardInitialText),
                               kAnyPad,
                               this);
    request.mMaxWidth = kKeyboardMaxWidth;
    request.mMaxLength = kKeyboardMaxLength;
    request.mTicker = kKeyboardTicker;
    MetKeyboardScreen::Open(request);
}

// 0x002a4910
void MetLoadNewFreqScreen::OnMsgScreenDismissed(const HxStr &name, int) {
    if (!(name == kNameRejectedDialogue)) {
        return;
    }
    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kLoadNewFreqScreen));
    ActivateNamedPanel(HxStr(kLoadNewFreqScreen));
}

// 0x002a3890
void MetLoadNewFreqScreen::EnterAndShow() {
    HxStr title = QueryConfigString(kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);
    mNameEntered = 0;

    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));

    MetLoadFreqBaseScreen::EnterAndShow();
}

// 0x002a84c0
void MetLoadNewFreqScreen::BeginExit() {
    if (mExitChoice != 0 && mButtonList->mSelected == kNameButtonIndex) {
        ExitScreenByName(HxStr(kHelpScreen));
    }

    MetScreen::BeginExit();
}

// 0x002a3978
void MetLoadNewFreqScreen::OnKeyboardDismissed() {
    if (mNameEntered != 0) {
        return;
    }

    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kLoadNewFreqScreen));
    ActivateNamedPanel(HxStr(kLoadNewFreqScreen));
}

// 0x002a4340
void MetLoadNewFreqScreen::OnKeyboardTextEntered(const HxStr &text) {
    mNameEntered = 1;
    HxStr name(text);
    if (name.mLen == 0) {
        name = (*mIdentityList)[mSelectedIdentity]->mAppearance.mUserName;
    }
    MetPersonaData *pIdentity = (*mIdentityList)[mSelectedIdentity];
    pIdentity->mAppearance.mUserName = name;
    HxStr birthday;
    if (FormatCurrentDateTime(birthday)) {
        pIdentity->mBirthday = birthday;
    }

    GameManagerImpl *pManager = Application::shared()->GetGameManager();
    pManager->ClearPersonas();
    Application::shared()->GetGameManager()->AddPersona(*pIdentity);
    MetPersonaData *pPersona = (*Application::shared()->GetGameManager()->GetPersonas())[0];
    GlobalSettings::shared(); // Yes, the binary discards this call's result.

    if (Application::shared()->GetGameManager()->GetGameMode() == kGameModeNet) {
        std::vector<HxStr> screens;
        screens.resize(kSaveReturnScreenCount);
        screens[kSaveReturnFirst] = kNetPortalScreen;
        screens[kSaveReturnGizmo] = kLeftGizmoScreen;
        screens[kSaveReturnHelp] = kHelpScreen;
        MetPersonaSaverScreen::StartSave(screens,
                                         pPersona,
                                         GlobalSettings::shared()->mCardSlots[0],
                                         kSaveConfirmReplace,
                                         kSaveIsCopy);
    } else {
        std::vector<HxStr> screens;
        screens.resize(kSaveReturnScreenCount);
        screens[kSaveReturnFirst] = kModeScreen;
        screens[kSaveReturnGizmo] = kLeftGizmoScreen;
        screens[kSaveReturnHelp] = kHelpScreen;
        MetPersonaSaverScreen::StartSave(screens,
                                         pPersona,
                                         GlobalSettings::shared()->mCardSlots[0],
                                         kSaveConfirmReplace,
                                         kSaveIsCopy);
    }
}

// 0x002a85c0
void MetLoadNewFreqScreen::UpdateNameLabel() {
    Rnd::Text *pLabel = mButtonList->ButtonAt(kNameButtonIndex)->mText;

    HxStr label = QueryConfigString(kLabelConfigCode, kNameLabelKey);
    pLabel->SetText(label);
}

// 0x002a3f80
void MetLoadNewFreqScreen::PrepareFreqMakerForSelection() {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
    MetFreqMakerButtonsScreen *pButtons =
        static_cast<MetFreqMakerButtonsScreen *>(FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
    pCanvas->LoadPrefab((*mIdentityList)[mSelectedIdentity], kNoRandomize);
    pButtons->SetEditing(kFreqMakerEditing);
    pButtons->mNewPersona = 1;
    MetFrontEndState::shared()->mReturnScreen = HxStr(kLoadNewFreqScreen);
}

// 0x002a8670
void MetLoadNewFreqScreen::OnCreateButton() {
    MetFrontEndState::shared()->mReturnScreen = HxStr(kLoadNewFreqScreen);
    MetLoadFreqBaseScreen::OnCreateButton();
}

// 0x002a8590
void MetLoadNewFreqScreen::AcquireIdentityList() {
    mIdentityList = MetFreqMakerAssetManager::shared()->GetIdentityList();
}

// 0x002a3b08
void MetLoadNewFreqScreen::BuildButtonList() {
    mButtonList->Clear();

    HxStr nameLabel = QueryConfigString(kLabelConfigCode, kNameLabelKey);
    mButtonList->Add(HxStr(kNameButtonObject), nameLabel);

    HxStr editLabel = QueryConfigString(kLabelConfigCode, kEditLabelKey);
    mButtonList->Add(HxStr(kEditButtonObject), editLabel);

    HxStr createLabel = QueryConfigString(kLabelConfigCode, kCreateLabelKey);
    mButtonList->Add(HxStr(kCreateButtonObject), createLabel);

    UpdateNameLabel();

    mHelpKeys.erase(mHelpKeys.begin(), mHelpKeys.end());
    mHelpKeys.push_back(HxStr(kNamePrompt));
    mHelpKeys.push_back(HxStr(kEditPrompt));
    mHelpKeys.push_back(HxStr(kCreatePrompt));

    mButtonList->SetSelected(kNameButtonIndex);
}
