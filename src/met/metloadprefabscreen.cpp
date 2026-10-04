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
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "rnd/button.h"
#include "rnd/text.h"
#include "rndartt/apalette.h"

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

#ifdef VIDEO_STANDARD_PAL
inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}
#endif

} // namespace

MetLoadPreFabScreen::MetLoadPreFabScreen(MetRenderer *pRenderer, int nPriority)
    : MetLoadFreqBaseScreen(pRenderer, nPriority) {
}

MetLoadPreFabScreen::~MetLoadPreFabScreen() {
    // Everything in the body is the teardown of mIdentities and the base.
}

void MetLoadPreFabScreen::EnterAndShow() {
    MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kPromptLayout));

    HxStr title = MetConfigText(kMetStrTPickChar, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    MetLoadFreqBaseScreen::EnterAndShow();
}

void MetLoadPreFabScreen::BuildButtonList() {
    mButtonList->Clear();
    mButtonList->Add(HxStr(kNameButtonObject), HxStr(kNoLabel));

    HxStr editLabel = MetConfigText(kMetStrPfEdit, kLabelConfigCode, kEditLabelKey);
    mButtonList->Add(HxStr(kEditButtonObject), editLabel);

    HxStr createLabel = MetConfigText(kMetStrPfCreate, kLabelConfigCode, kCreateLabelKey);
    mButtonList->Add(HxStr(kCreateButtonObject), createLabel);

    mHelpKeys.erase(mHelpKeys.begin(), mHelpKeys.end());
    mHelpKeys.push_back(MetText(kMetStrHIdName, kNamePrompt));
    mHelpKeys.push_back(MetText(kMetStrHCidEdit, kEditPrompt));
    mHelpKeys.push_back(MetText(kMetStrHIdCreate, kCreatePrompt));

    mButtonList->SetSelected(kNameButtonIndex);
}

void MetLoadPreFabScreen::UpdateNameLabel() {
    HxStr username((*mIdentityList)[mSelectedIdentity]->mAppearance.mUserName);
    mButtonList->GetButton(kNameButtonIndex)->mText->SetText(username);

#ifdef VIDEO_STANDARD_PAL
    HxStr editLabel = GetMetString(kMetStrPfEdit);
    HxStr editText(Rnd::MakeString(TextOrEmpty(editLabel), TextOrEmpty(username)));
#else
    HxStr editLabel = QueryConfigString(kLabelConfigCode, kEditLabelKey);
    HxStr editLabelWithName(editLabel);
    HxStr editText(editLabelWithName += username);
#endif
    mButtonList->GetButton(kEditButtonIndex)->mText->SetText(editText);
}

void MetLoadPreFabScreen::AcquireIdentityList() {
    mIdentities.erase(mIdentities.begin(), mIdentities.end());
    for (unsigned int i = 0; i < MetPersonaData::savedList()->size(); ++i) {
        mIdentities.push_back((*MetPersonaData::savedList())[i]);
    }
    for (unsigned int i = 0; i < MetFreqMakerAssetManager::shared()->GetIdentityList()->size();
         ++i) {
        mIdentities.push_back((*MetFreqMakerAssetManager::shared()->GetIdentityList())[i]);
    }
    mIdentityList = &mIdentities;
}

void MetLoadPreFabScreen::PrepareFreqMakerForSelection() {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
    MetFreqMakerButtonsScreen *pButtons =
        static_cast<MetFreqMakerButtonsScreen *>(FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
    MetPersonaData *pPersona = (*mIdentityList)[mSelectedIdentity];
    if (static_cast<unsigned>(mSelectedIdentity) < MetPersonaData::savedList()->size()) {
        pCanvas->LoadPersona(pPersona);
    } else {
        pCanvas->LoadPrefab(pPersona, kNoRandomize);
    }
    pButtons->SetEditing(kFreqMakerEditing);
    pButtons->mNewPersona = 0;
    MetFrontEndState::shared()->mReturnScreen = HxStr(kLoadPreFabScreen);
}

void MetLoadPreFabScreen::OnNameButton() {
    if (Application::shared()->GetGameManager()->GetGameMode() == kNetworkGameMode) {
        Fatal(kNetModeError);
        return;
    }

    MetPersonaData *pPersona = (*mIdentityList)[mSelectedIdentity];
    Application::shared()->GetGameManager()->ClearPersonas();
    Application::shared()->GetGameManager()->AddPersona(*pPersona);
    PushNamedScreen(HxStr(kLeftGizmoScreen));
    PushNamedScreen(HxStr(kModeScreen));
    ActivateNamedPanel(HxStr(kModeScreen));
}

void MetLoadPreFabScreen::OnCreateButton() {
    if (MetPersonaData::savedList()->size() >= kMaxSavedPersonas) {
        ExitScreenByName(HxStr(kHelpScreen));
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
        HxStr text = MetConfigText(kMetStrNomemFreqLimit, kLabelConfigCode, kFreqLimitTextKey);
        MetMsgScreen::Show(HxStr(kFreqLimitMessage),
                           MetText(kMetStrMsgERROR, kFreqLimitTitle),
                           text,
                           kOneButton,
                           buttons,
                           this);
    } else {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kLoadPreFabScreen);
#ifdef VIDEO_STANDARD_PAL
        OpenFreqMakerForCreate();
#else
        MetLoadFreqBaseScreen::OnCreateButton();
#endif
    }
}

void MetLoadPreFabScreen::OnMsgScreenDismissed(const HxStr &name, int) {
    if (!(name == kFreqLimitMessage)) {
        return;
    }

    HxStr title = MetConfigText(kMetStrTPickChar, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kLoadPreFabScreen));
    ActivateNamedPanel(HxStr(kLoadPreFabScreen));
}

MetScreen *MetLoadPreFabScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLoadPreFabScreen(pRenderer, nPriority);
}
