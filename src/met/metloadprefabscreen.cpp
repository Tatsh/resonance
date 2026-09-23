#include "met/metloadprefabscreen.h"

#include <vector>

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "met/metbuttonlist.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakerbuttonsscreen.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metmsgscreen.h"
#include "met/metpersonadata.h"
#include "met/metscreentitlescreen.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "rnd/button.h"
#include "rnd/text.h"
#include "rndartt/apalette.h"
#include "script/configquery.h"

namespace {

static const char *const kFreqMakerCanvasScreen = "MetFreqMakerCanvasScreen";
static const char *const kFreqMakerButtonsScreen = "MetFreqMakerButtonsScreen";
static const char *const kLoadPreFabScreen = "MetLoadPreFabScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kModeScreen = "MetModeScreen";

// The MetFreqMakerButtonsScreen::SetEditing() value, and the LoadPrefab() randomise flag.
constexpr int kFreqMakerEditing = 1;
constexpr int kNoRandomize = 0;

// The prompt layout and title EnterAndShow() and OnMsgScreenDismissed() select.
static const char *const kPromptLayout = "standard_title";
constexpr int kTitleConfigCode = 0x269;
static const char *const kTitleKey = "pick_char";

// The three buttons BuildButtonList() appends, in ring order, and their labels.
static const char *const kNameButtonObject = "cid_01.but";
static const char *const kEditButtonObject = "cid_02.but";
static const char *const kCreateButtonObject = "cid_03.but";
constexpr int kLabelConfigCode = 0x258;
static const char *const kNoLabel = "";
static const char *const kEditLabelKey = "pf_edit";
static const char *const kCreateLabelKey = "pf_create";

// The three prompts BuildButtonList() appends, matching the button order above.
static const char *const kNamePrompt = "id_name";
static const char *const kEditPrompt = "cid_edit";
static const char *const kCreatePrompt = "id_create";

// Ring indices BuildButtonList() and UpdateNameLabel() address.
constexpr int kNameButtonIndex = 0;
constexpr int kEditButtonIndex = 1;

// Game mode that makes OnNameButton() fail.
constexpr int kNetworkGameMode = 3;
static const char *const kNetModeError =
    "We arrived at the pre-fab load (no mem card) screen in net mode.";

// The saved personas the memory card has room for.
constexpr unsigned kMaxSavedPersonas = 8;

// The dialogue OnCreateButton() shows and OnMsgScreenDismissed() responds to.
static const char *const kFreqLimitMessage = "freq_limit";
static const char *const kFreqLimitTitle = "ERROR";
static const char *const kFreqLimitTextKey = "nomem_freq_limit";
static const char *const kOkButton = "OK";
constexpr int kOneButton = 1;

} // namespace

MetLoadPreFabScreen::MetLoadPreFabScreen(MetRenderer *pRenderer, int nPriority)
    : MetLoadFreqBaseScreen(pRenderer, nPriority) {
}

// 0x002a8aa0
void MetLoadPreFabScreen::EnterAndShow() {
    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));

    HxStr title;
    QueryConfigString(&title, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    MetLoadFreqBaseScreen::EnterAndShow();
}

// 0x002a8b80
void MetLoadPreFabScreen::BuildButtonList() {
    mUnknown90->Clear();
    mUnknown90->Add(HxStr(kNameButtonObject), HxStr(kNoLabel));

    HxStr editLabel;
    QueryConfigString(&editLabel, kLabelConfigCode, kEditLabelKey);
    mUnknown90->Add(HxStr(kEditButtonObject), editLabel);

    HxStr createLabel;
    QueryConfigString(&createLabel, kLabelConfigCode, kCreateLabelKey);
    mUnknown90->Add(HxStr(kCreateButtonObject), createLabel);

    mUnknown38.erase(mUnknown38.begin(), mUnknown38.end());
    mUnknown38.push_back(HxStr(kNamePrompt));
    mUnknown38.push_back(HxStr(kEditPrompt));
    mUnknown38.push_back(HxStr(kCreatePrompt));

    mUnknown90->SetSelected(kNameButtonIndex);
}

// 0x002a8fe0
void MetLoadPreFabScreen::UpdateNameLabel() {
    HxStr username((*mUnknown8c)[mUnknown94]->mUnknown140.mUnknown00);
    mUnknown90->ButtonAt(kNameButtonIndex)->mText->SetText(username);

    HxStr editLabel;
    QueryConfigString(&editLabel, kLabelConfigCode, kEditLabelKey);
    HxStr editLabelWithName(editLabel);
    HxStr editText(editLabelWithName += username);
    mUnknown90->ButtonAt(kEditButtonIndex)->mText->SetText(editText);
}

// 0x002a91b8
void MetLoadPreFabScreen::AcquireIdentityList() {
    mIdentities.erase(mIdentities.begin(), mIdentities.end());
    for (unsigned int i = 0; i < MetPersonaData::savedList()->size(); ++i) {
        mIdentities.push_back((*MetPersonaData::savedList())[i]);
    }
    for (unsigned int i = 0; i < MetFreqMakerAssetManager::shared()->GetIdentityList()->size();
         ++i) {
        mIdentities.push_back((*MetFreqMakerAssetManager::shared()->GetIdentityList())[i]);
    }
    mUnknown8c = &mIdentities;
}

// 0x002a9330
void MetLoadPreFabScreen::PrepareFreqMakerForSelection() {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
    MetFreqMakerButtonsScreen *pButtons =
        static_cast<MetFreqMakerButtonsScreen *>(FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
    MetPersonaData *pPersona = (*mUnknown8c)[mUnknown94];
    if (static_cast<unsigned>(mUnknown94) < MetPersonaData::savedList()->size()) {
        pCanvas->LoadPersona(pPersona);
    } else {
        pCanvas->LoadPrefab(pPersona, kNoRandomize);
    }
    pButtons->SetEditing(kFreqMakerEditing);
    pButtons->mNewPersona = 0;
    MetFrontEndState::shared()->mUnknown24 = HxStr(kLoadPreFabScreen);
}

// 0x002a94f0
void MetLoadPreFabScreen::OnNameButton() {
    if (Application::shared()->GetGameManager()->GetGameMode() == kNetworkGameMode) {
        Fatal(kNetModeError);
        return;
    }

    MetPersonaData *pPersona = (*mUnknown8c)[mUnknown94];
    Application::shared()->GetGameManager()->ClearPersonas();
    Application::shared()->GetGameManager()->AddPersona(*pPersona);
    PushNamedScreen(HxStr(kLeftGizmoScreen));
    PushNamedScreen(HxStr(kModeScreen));
    ActivateNamedPanel(HxStr(kModeScreen));
}

// 0x002a9710
void MetLoadPreFabScreen::OnCreateButton() {
    if (MetPersonaData::savedList()->size() >= kMaxSavedPersonas) {
        ExitScreenByName(HxStr(kHelpScreen));
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kOkButton));
        HxStr text;
        QueryConfigString(&text, kLabelConfigCode, kFreqLimitTextKey);
        MetMsgScreen::Show(
            HxStr(kFreqLimitMessage), HxStr(kFreqLimitTitle), text, kOneButton, buttons, this);
    } else {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kLoadPreFabScreen);
        MetLoadFreqBaseScreen::OnCreateButton();
    }
}

// 0x002a9b38
void MetLoadPreFabScreen::OnMsgScreenDismissed(const HxStr &name, int) {
    if (!(name == kFreqLimitMessage)) {
        return;
    }

    HxStr title;
    QueryConfigString(&title, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kLoadPreFabScreen));
    ActivateNamedPanel(HxStr(kLoadPreFabScreen));
}

// 0x002ad3c8
MetScreen *MetLoadPreFabScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLoadPreFabScreen(pRenderer, nPriority);
}
