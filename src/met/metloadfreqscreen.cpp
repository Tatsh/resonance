#include "met/metloadfreqscreen.h"

#include <vector>

#include "app/application.h"
#include "app/playsound.h"
#include "game/gamemanagerimpl.h"
#include "game/globalsettings.h"
#include "met/metfreqmakerbuttonsscreen.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metmsgscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

// mUnknowna8 as the constructor leaves it.
constexpr int kInitialUnknowna8 = 1;

// The three buttons BuildButtonList() appends, in ring order.
static const char *const kNameButtonObject = "cid_01.but";
static const char *const kEditButtonObject = "cid_02.but";
static const char *const kCreateButtonObject = "cid_03.but";

// The three prompts BuildButtonList() appends, matching the button order above.
static const char *const kNamePrompt = "id_name";
static const char *const kEditPrompt = "cid_edit";
static const char *const kCreatePrompt = "id_create";

// Configuration code every label and the title come from, with the key each one passes.
constexpr int kLabelConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;
static const char *const kEditLabelKey = "lf_edit";
static const char *const kCreateLabelKey = "lf_create";
static const char *const kTitleKey = "load_char";

// The prompt layout EnterAndShow() selects.
static const char *const kPromptLayout = "standard_title";

// Screens the class pushes and activates by registry key.
static const char *const kLoadFreqScreen = "MetLoadFreqScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kNetPortalScreen = "MetNetPortalScreen";
static const char *const kModeScreen = "MetModeScreen";
static const char *const kFreqMakerCanvasScreen = "MetFreqMakerCanvasScreen";
static const char *const kFreqMakerButtonsScreen = "MetFreqMakerButtonsScreen";

// The MetFreqMakerButtonsScreen::SetEditing() value PrepareFreqMakerForSelection() passes.
constexpr int kFreqMakerEditing = 1;

// The message screen OnCreateButton() shows and OnMsgScreenDismissed() responds to.
static const char *const kFreqLimitMessage = "freq_limit";

// OnCreateButton()'s two refusals: the identity limit and its text, the text for a card without
// room, the dialogue title and its one button, and the screen it opens otherwise.
constexpr unsigned kMaxIdentities = 8;
static const char *const kNoSpaceText = "freq_no_space";
static const char *const kErrorTitle = "ERROR";
static const char *const kOkButton = "OK";
constexpr int kOneButton = 1;
static const char *const kFreqCreateScreen = "MetFreqCreateScreen";

// The sound slot 33 plays.
static const char *const kSelectFreqSound = "SND_MET_SELECTFREQ";

// The empty literal the first button's label passes.
static const char *const kNoLabel = "";

// Ring indices BuildButtonList() and UpdateNameLabel() address.
constexpr int kNameButtonIndex = 0;
constexpr int kEditButtonIndex = 1;

// Game mode that sends OnNameButton() to the network portal rather than to mode select.
constexpr int kNetworkGameMode = 3;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A dialogue text read by value from configuration.
inline HxStr ConfigText(const char *pszKey) {
    HxStr value;
    QueryConfigString(&value, kLabelConfigCode, pszKey);
    return value;
}

} // namespace

// 0x0029bcf0
MetLoadFreqScreen::MetLoadFreqScreen(MetRenderer *pRenderer, int nPriority)
    : MetLoadFreqBaseScreen(pRenderer, nPriority) {
    mUnknowna8 = kInitialUnknowna8;
}

// 0x0029bd38
MetLoadFreqScreen::~MetLoadFreqScreen() {
}

// 0x00297c10
void MetLoadFreqScreen::OnCreateButton() {
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    if (mUnknown8c->size() >= kMaxIdentities) {
        ExitScreenByName(HxStr(kHelpScreen));
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kOkButton));
        const HxStr format(ConfigText(kFreqLimitMessage));
        const HxStr text(
            FormatString(TextOrEmpty(format),
                         kMaxIdentities,
                         TextOrEmpty(GlobalSettings::shared()->mCardSlots[0].mSlotName)));
        MetMsgScreen::Show(
            HxStr(kFreqLimitMessage), HxStr(kErrorTitle), text, kOneButton, buttons, this);
        return;
    }
    if (GlobalSettings::shared()->mCardSlots[0].mFree < GlobalSettings::shared()->mUnknown74) {
        ExitScreenByName(HxStr(kHelpScreen));
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kOkButton));
        const HxStr format(ConfigText(kNoSpaceText));
        const HxStr text(
            FormatString(TextOrEmpty(format),
                         TextOrEmpty(GlobalSettings::shared()->mCardSlots[0].mSlotName),
                         GlobalSettings::shared()->mUnknown74));
        MetMsgScreen::Show(
            HxStr(kFreqLimitMessage), HxStr(kErrorTitle), text, kOneButton, buttons, this);
        return;
    }
    PushNamedScreen(HxStr(kFreqCreateScreen));
    ActivateNamedPanel(HxStr(kFreqCreateScreen));
}

// 0x0029bc68
MetScreen *MetLoadFreqScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLoadFreqScreen(pRenderer, nPriority);
}

// 0x00297448
void MetLoadFreqScreen::EnterAndShow() {
    HxStr title;
    QueryConfigString(&title, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));

    MetLoadFreqBaseScreen::EnterAndShow();
}

// 0x00298510
void MetLoadFreqScreen::OnMsgScreenDismissed(const HxStr &name, int) {
    if (!(name == kFreqLimitMessage)) {
        return;
    }

    HxStr title;
    QueryConfigString(&title, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);

    PushNamedScreen(HxStr(kLoadFreqScreen));
    PushNamedScreen(HxStr(kHelpScreen));
    mUnknowna8 = 0;
    ActivateNamedPanel(HxStr(kLoadFreqScreen));
}

// 0x0029bdc8
void MetLoadFreqScreen::OnUnknownSlot33() {
    PlaySoundByName(kSelectFreqSound);
}

// 0x002976f8
void MetLoadFreqScreen::UpdateNameLabel() {
    HxStr username((*mUnknown8c)[mUnknown94]->mUnknown140.mUnknown00);
    mUnknown90->ButtonAt(kNameButtonIndex)->mText->SetText(username);

    HxStr editLabel;
    QueryConfigString(&editLabel, kLabelConfigCode, kEditLabelKey);
    HxStr editLabelWithName(editLabel);
    HxStr editText(editLabelWithName += username);
    mUnknown90->ButtonAt(kEditButtonIndex)->mText->SetText(editText);
}

// 0x002978d0
void MetLoadFreqScreen::OnNameButton() {
    MetPersonaData *pPersona = (*mUnknown8c)[mUnknown94];

    Application::shared()->GetGameManager()->ClearPersonas();
    Application::shared()->GetGameManager()->AddPersona(*pPersona);

    if (Application::shared()->GetGameManager()->GetGameMode() == kNetworkGameMode) {
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kNetPortalScreen));
        ActivateNamedPanel(HxStr(kNetPortalScreen));
    } else {
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kModeScreen));
        ActivateNamedPanel(HxStr(kModeScreen));
    }
}

// 0x00297528
void MetLoadFreqScreen::PrepareFreqMakerForSelection() {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
    MetFreqMakerButtonsScreen *pButtons =
        static_cast<MetFreqMakerButtonsScreen *>(FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
    MetPersonaData *pPersona = (*mUnknown8c)[mUnknown94];
    pCanvas->LoadPersona(pPersona);
    pButtons->SetEditing(kFreqMakerEditing);
    pButtons->mNewPersona = 0;
    Application::shared()->GetGameManager()->ClearPersonas();
    Application::shared()->GetGameManager()->AddPersona(*pPersona);
    MetFrontEndState::shared()->mUnknown24 = HxStr(kLoadFreqScreen);
}

// 0x0029bda0
void MetLoadFreqScreen::AcquireIdentityList() {
    mUnknown8c = MetPersonaData::loadList();
}

// 0x00296fe8
void MetLoadFreqScreen::BuildButtonList() {
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
