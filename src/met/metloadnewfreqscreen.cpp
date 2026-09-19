#include "met/metloadnewfreqscreen.h"

#include "met/metfreqmakerassetmanager.h"
#include "met/methelpscreen.h"
#include "met/metscreentitlescreen.h"
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
constexpr int kLabelConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;
static const char *const kNameLabelKey = "nf_enter";
static const char *const kEditLabelKey = "nf_edit";
static const char *const kCreateLabelKey = "nf_create";
static const char *const kTitleKey = "create_char";

// The prompt layout EnterAndShow() selects.
static const char *const kPromptLayout = "standard_title";

// Screens the class pushes, exits, and activates by registry key.
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kLoadNewFreqScreen = "MetLoadNewFreqScreen";

// Ring index of the button BuildButtonList() selects and UpdateNameLabel() labels.
constexpr int kNameButtonIndex = 0;

} // namespace

MetLoadNewFreqScreen::MetLoadNewFreqScreen(MetRenderer *pRenderer, int nPriority)
    : MetLoadFreqBaseScreen(pRenderer, nPriority) {
}

void MetLoadNewFreqScreen::EnterAndShow() {
    HxStr title;
    QueryConfigString(&title, kTitleConfigCode, kTitleKey);
    MetScreenTitleScreen::SetTitle(title);
    mUnknowna8 = 0;

    MetHelpScreen::SelectPreset(HxStr(kPromptLayout));

    MetLoadFreqBaseScreen::EnterAndShow();
}

void MetLoadNewFreqScreen::BeginExit() {
    if (mUnknown18 != 0 && mUnknown90->mSelected == kNameButtonIndex) {
        ExitScreenByName(HxStr(kHelpScreen));
    }

    MetScreen::BeginExit();
}

void MetLoadNewFreqScreen::OnKeyboardDismissed() {
    if (mUnknowna8 != 0) {
        return;
    }

    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kLoadNewFreqScreen));
    ActivateNamedPanel(HxStr(kLoadNewFreqScreen));
}

void MetLoadNewFreqScreen::UpdateNameLabel() {
    Rnd::Text *pLabel = mUnknown90->ButtonAt(kNameButtonIndex)->mText;

    HxStr label;
    QueryConfigString(&label, kLabelConfigCode, kNameLabelKey);
    pLabel->SetText(label);
}

void MetLoadNewFreqScreen::AcquireIdentityList() {
    mUnknown8c = MetFreqMakerAssetManager::shared()->GetIdentityList();
}

void MetLoadNewFreqScreen::BuildButtonList() {
    mUnknown90->Clear();

    HxStr nameLabel;
    QueryConfigString(&nameLabel, kLabelConfigCode, kNameLabelKey);
    mUnknown90->Add(HxStr(kNameButtonObject), nameLabel);

    HxStr editLabel;
    QueryConfigString(&editLabel, kLabelConfigCode, kEditLabelKey);
    mUnknown90->Add(HxStr(kEditButtonObject), editLabel);

    HxStr createLabel;
    QueryConfigString(&createLabel, kLabelConfigCode, kCreateLabelKey);
    mUnknown90->Add(HxStr(kCreateButtonObject), createLabel);

    UpdateNameLabel();

    mUnknown38.erase(mUnknown38.begin(), mUnknown38.end());
    mUnknown38.push_back(HxStr(kNamePrompt));
    mUnknown38.push_back(HxStr(kEditPrompt));
    mUnknown38.push_back(HxStr(kCreatePrompt));

    mUnknown90->SetSelected(kNameButtonIndex);
}
