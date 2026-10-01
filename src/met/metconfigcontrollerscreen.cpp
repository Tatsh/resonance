#include "met/metconfigcontrollerscreen.h"

#include <vector>

#include "game/controllerconfig.h"
#include "game/globalsettings.h"
#include "met/metfrontendstate.h"
#include "met/metglobalsettingssaverscreen.h"
#include "met/methelpscreen.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "psx";
static const char *const kDirectory = "metagame/shared";
static const char *const kContainerName = "psx_config";

// The configuration codes the localised prompts and titles are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The configuration rows, in the order of every per-row table below.
enum Row {
    kRowLeftNote1 = 0,
    kRowLeftNote2 = 1,
    kRowCenterNote1 = 2,
    kRowCenterNote2 = 3,
    kRowRightNote1 = 4,
    kRowRightNote2 = 5,
    kRowEraseAndPower = 6,
    kRowExpression = 7,
    kRowRemixFX = 8,
    kRowCount = 9,
};

// The button indices ControllerConfig::SetButton() takes. The eight buttons below the sticks are
// the code less mFirstButtonCode.
enum Button {
    kButtonNone = -1,
    kButtonLeftStick = 8,
    kButtonRightStick = 9,
    kButtonCount = 10,
};

// The button codes the value texts show. The first-note rows cycle L1 through R2, the second-note
// rows square through cross, and the erase and power row every button.
constexpr char kCodeSquare = 'a';
constexpr char kCodeCross = 'd';
constexpr char kCodeL1 = 'e';
constexpr char kCodeR2 = 'h';
constexpr char kCodeUnassigned = 'o';

// A command code above MetScreenCommandCode's range, which this screen alone gives a meaning.
constexpr int kCommandRestoreDefaults = 7;

// MetScreen::mExitChoice on exit, read back by OnExitFinished().
constexpr int kExitCancelled = 0;
constexpr int kExitStored = 2;

constexpr int kFrontEndFlagSet = 1;
constexpr int kOneButton = 1;

// The localisation keys of the rows, which are also each row's help prompt.
const char *const kRowKeys[] = {
    "controller_config_left_note_1",
    "controller_config_left_note_2",
    "controller_config_center_note_1",
    "controller_config_center_note_2",
    "controller_config_right_note_1",
    "controller_config_right_note_2",
    "controller_config_erase_and_power",
    "controller_config_expression",
    "controller_config_remix_fx",
};

const char *const kRowButtons[] = {
    "psx_leftnote_01.but",
    "psx_leftnote_02.but",
    "psx_centernote_01.but",
    "psx_centernote_02.but",
    "psx_rightnote_01.but",
    "psx_rightnote_02.but",
    "psx_erasepower.but",
    "psx_express.but",
    "psx_remixfx.but",
};

const char *const kButtonMeshes[] = {
    "psx_square_hi.mesh",
    "psx_triangle_hi.mesh",
    "psx_circ_hi.mesh",
    "psx_x_hi.mesh",
    "psx_l1_hi.mesh",
    "psx_l2_hi.mesh",
    "psx_r1_hi.mesh",
    "psx_r2_hi.mesh",
    "psx_analogl_hi.mesh",
    "psx_analogr_hi.mesh",
};

const char *const kRowValueTexts[] = {
    "psx_leftnote_val_01.txt",
    "psx_leftnote_val_02.txt",
    "psx_centernote_val_01.txt",
    "psx_centernote_val_02.txt",
    "psx_rightnote_val_01.txt",
    "psx_rightnote_val_02.txt",
    "psx_erasepower_val.txt",
    "psx_express_val.txt",
    "psx_remixfx_val.txt",
};

static const char *const kInstructionText1 = "psx_instruct_01_val.txt";
static const char *const kInstructionKey1 = "controller_config_instruct1";
static const char *const kInstructionText2 = "psx_instruct_02_val.txt";
static const char *const kInstructionKey2 = "controller_config_instruct2";

static const char *const kPlayerKey = "config_controller_player";
static const char *const kOptionsKey = "config_controller_options";
static const char *const kTitleFormat = "%s %d %s";
static const char *const kSaveBackPreset = "cc_save_back";
static const char *const kStandardPreset = "standard_title";

static const char *const kPauseGameScreen = "MetPauseSoloGameScreen";
static const char *const kPauseRemixScreen = "MetPauseSoloRemixScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kRightGizmoScreen = "MetRightGizmoScreen";
static const char *const kOptionsButtonsScreen = "MetConfigOptionsButtonsScreen";
static const char *const kThisScreen = "MetConfigControllerScreen";

static const char *const kMissingValuesDialogue = "missingconfigvals";
static const char *const kNoMemcardDialogue = "nomemcard";
static const char *const kErrorTitle = "ERROR";
static const char *const kMissingValuesText = "You must choose a button for every selection.";
static const char *const kOkButton = "OK";

// 0x00891a90
HxStr g_abLeftStickName("left analog stick");
// 0x00891a98
HxStr g_abRightStickName("right analog stick");
// 0x00891aa0
// The value of a row with no button.
HxStr g_abUnassignedName("o");

inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr text = QueryConfigString(nCode, pszKey);
    return text;
}

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

inline bool IsStickRow(int nRow) {
    return nRow == kRowExpression || nRow == kRowRemixFX;
}

// Swaps a stick row between the two sticks.
inline void ToggleStick(Rnd::Text *pText) {
    if (pText->mPreWrapText == g_abLeftStickName) {
        pText->SetText(g_abRightStickName);
    } else {
        pText->SetText(g_abLeftStickName);
    }
}

} // namespace

// 0x001ff1e0
MetConfigControllerScreen::MetConfigControllerScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreenMultiSoundBank(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mRows(nullptr), mFirstButtonCode(kCodeSquare), mLastButtonCode(kCodeR2), mControllerIndex(0) {
    mRows = new MetButtonList();

    // The binary expands each loop in this constructor into one call per entry.
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mHelpKeys.push_back(HxStr(kRowKeys[nRow]));
    }

    mButtonMeshes.resize(kButtonCount);
    mButtonMeshNames.resize(kButtonCount);
    for (int nButton = 0; nButton < kButtonCount; ++nButton) {
        mButtonMeshNames[nButton] = kButtonMeshes[nButton];
    }

    mRowValueTexts.resize(kRowCount);
    mRowValueTextNames.resize(kRowCount);
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mRowValueTextNames[nRow] = kRowValueTexts[nRow];
    }
}

// 0x002008e8
MetConfigControllerScreen::~MetConfigControllerScreen() {
    delete mRows;
}

// 0x001fff40
void MetConfigControllerScreen::ResolveContainerViews() {
    MetScreenMultiSoundBank::ResolveContainerViews();

    // The binary expands this loop into one call per row.
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mRows->Add(HxStr(kRowButtons[nRow]), ConfigText(kPromptConfigCode, kRowKeys[nRow]));
    }

    // Yes, the binary does not test either instruction text for null.
    dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kInstructionText1)))
        ->SetText(ConfigText(kPromptConfigCode, kInstructionKey1));
    dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(kInstructionText2)))
        ->SetText(ConfigText(kPromptConfigCode, kInstructionKey2));

    for (int nButton = 0; nButton < kButtonCount; ++nButton) {
        mButtonMeshes[nButton] =
            dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(mButtonMeshNames[nButton]));
        mButtonMeshes[nButton]->SetShowing(false);
    }

    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mRowValueTexts[nRow] =
            dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(mRowValueTextNames[nRow]));
    }
}

// 0x00200ba8
void MetConfigControllerScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (pCommand->mPadIndex != mControllerIndex + 1) {
        return;
    }

    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        ClearDuplicateAssignment(mRows->mSelected);
        mRows->SelectPrevious();
        MetHelpScreen::SetText(mHelpKeys[mRows->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandNext:
        ClearDuplicateAssignment(mRows->mSelected);
        mRows->SelectNext();
        MetHelpScreen::SetText(mHelpKeys[mRows->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandLeft: {
        const int nRow = mRows->mSelected;
        Rnd::Text *pText = mRowValueTexts[nRow];
        if (IsStickRow(nRow)) {
            ToggleStick(pText);
        } else {
            pText->SetText(HxStr(1, PreviousButtonCode(nRow, pText->mPreWrapText[0])));
        }
        break;
    }

    case kMetScreenCommandRight: {
        const int nRow = mRows->mSelected;
        Rnd::Text *pText = mRowValueTexts[nRow];
        if (IsStickRow(nRow)) {
            ToggleStick(pText);
        } else {
            pText->SetText(HxStr(1, NextButtonCode(nRow, pText->mPreWrapText[0])));
        }
        break;
    }

    case kMetScreenCommandSelect:
        ClearDuplicateAssignment(mRows->mSelected);
        if (!AllRowsAssigned()) {
            std::vector<HxStr> buttons;
            buttons.push_back(HxStr(kOkButton));
            MetMsgScreen::Show(HxStr(kMissingValuesDialogue),
                               HxStr(kErrorTitle),
                               HxStr(kMissingValuesText),
                               kOneButton,
                               buttons,
                               this);
            return;
        }
        StoreConfig();
        mExitChoice = kExitStored;
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        return;

    case kMetScreenCommandBack:
        mExitChoice = kExitCancelled;
        if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen ||
            MetFrontEndState::shared()->mReturnScreen == kPauseRemixScreen) {
            ExitScreenByName(HxStr(kHelpScreen));
            ExitScreenByName(HxStr(kTitleScreen));
        }
        BeginExit();
        return;

    case kCommandRestoreDefaults: {
        MetScreenMultiSoundBank::PlaySlideSound(pCommand->mPadIndex);
        ControllerConfig defaults;
        ShowConfig(defaults);
        return;
    }

    default:
        return;
    }

    UpdateButtonHighlight();
}

// 0x00201478
void MetConfigControllerScreen::EnterAndShow() {
    mRows->SetSelected(0);

    HxStr title;
    HxStr options;
    HxStr player;
    player = ConfigText(kPromptConfigCode, kPlayerKey);
    options = ConfigText(kTitleConfigCode, kOptionsKey);
    title =
        FormatString(kTitleFormat, TextOrEmpty(player), mControllerIndex + 1, TextOrEmpty(options));
    MetScreenTitleScreen::SetTitle(title);

    MetHelpScreen::SetText(mHelpKeys[mRows->mSelected], mRenderer->mAnimationFrame);
    if (MetFrontEndState::shared()->mUsingMemcard != 0) {
        MetHelpScreen::SelectPreset(HxStr(kSaveBackPreset));
    } else {
        MetHelpScreen::SelectPreset(HxStr(kStandardPreset));
    }

    ShowConfig(GlobalSettings::shared()->mControllers[mControllerIndex]);
    MetScreenMultiSoundBank::EnterAndShow();
}

// 0x00201790
void MetConfigControllerScreen::OnExitFinished() {
    if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen ||
        MetFrontEndState::shared()->mReturnScreen == kPauseRemixScreen) {
        if (MetFrontEndState::shared()->mUsingMemcard == kFrontEndFlagSet &&
            mExitChoice != kExitCancelled) {
            MetFrontEndState::shared()->mSettingsDirty = kFrontEndFlagSet;
        }
        if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen) {
            PushNamedScreen(HxStr(kPauseGameScreen));
            ActivateNamedPanel(HxStr(kPauseGameScreen));
        } else {
            PushNamedScreen(HxStr(kPauseRemixScreen));
            ActivateNamedPanel(HxStr(kPauseRemixScreen));
        }
        MetFrontEndState::shared()->mReturnScreen = HxStr("");
        return;
    }

    switch (mExitChoice) {
    case kExitCancelled:
        PushNamedScreen(HxStr(kRightGizmoScreen));
        PushNamedScreen(HxStr(kOptionsButtonsScreen));
        ActivateNamedPanel(HxStr(kOptionsButtonsScreen));
        break;

    case kExitStored: {
        std::vector<HxStr> screens;
        screens.push_back(HxStr(kOptionsButtonsScreen));
        screens.push_back(HxStr(kRightGizmoScreen));
        screens.push_back(HxStr(kHelpScreen));
        MetGlobalSettingsSaverScreen::StartSave(screens);
        break;
    }

    default:
        break;
    }
}

// 0x00201eb8
void MetConfigControllerScreen::ClearDuplicateAssignment(int nRow) {
    if (IsStickRow(nRow)) {
        Rnd::Text *pOther = mRowValueTexts[nRow == kRowExpression ? kRowRemixFX : kRowExpression];
        const HxStr &value = mRowValueTexts[nRow]->mPreWrapText;
        if (value == g_abLeftStickName) {
            pOther->SetText(g_abRightStickName);
        } else if (value == g_abRightStickName) {
            pOther->SetText(g_abLeftStickName);
        }
        return;
    }

    const char code = mRowValueTexts[nRow]->mPreWrapText[0];
    const int nCount = mRowValueTexts.size();
    for (int i = 0; i < nCount; ++i) {
        if (i == nRow || i == kRowExpression || i == kRowRemixFX) {
            continue;
        }
        Rnd::Text *pText = mRowValueTexts[i];
        if (pText->mPreWrapText[0] == code) {
            pText->SetText(g_abUnassignedName);
        }
    }
}

// 0x00202070
void MetConfigControllerScreen::UpdateButtonHighlight() {
    const int nRow = mRows->mSelected;
    HxStr value(mRowValueTexts[nRow]->mPreWrapText);
    if (IsStickRow(nRow)) {
        SetButtonHighlights(value == g_abLeftStickName ? kButtonLeftStick : kButtonRightStick);
        return;
    }

    const int nButton = value[0] - mFirstButtonCode;
    SetButtonHighlights(static_cast<unsigned>(nButton) < kButtonCount ? nButton : kButtonNone);
}

// 0x002022a0
void MetConfigControllerScreen::ShowConfig(ControllerConfig &config) {
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        const int nButton = config.GetButtonIndex(nRow);
        Rnd::Text *pText = mRowValueTexts[nRow];
        if (nButton == kButtonLeftStick) {
            pText->SetText(g_abLeftStickName);
        } else if (nButton == kButtonRightStick) {
            pText->SetText(g_abRightStickName);
        } else {
            pText->SetText(HxStr(1, static_cast<char>(mFirstButtonCode + nButton)));
        }
    }
}

// 0x00206828
MetConfigControllerScreen *MetConfigControllerScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetConfigControllerScreen(pRenderer, nPriority);
}

// 0x002068b0
void MetConfigControllerScreen::PlaySlideSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlaySlideSound(nSelector);
    }
}

// 0x002068e0
void MetConfigControllerScreen::PlayCycleLeftSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlayCycleLeftSound(nSelector);
    }
}

// 0x00206910
void MetConfigControllerScreen::PlayCycleRightSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlayCycleRightSound(nSelector);
    }
}

// 0x00206940
void MetConfigControllerScreen::PlayHighSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlayHighSound(nSelector);
    }
}

// 0x00206970
void MetConfigControllerScreen::PlayLeaveSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlayLeaveSound(nSelector);
    }
}

// 0x002069a0
void MetConfigControllerScreen::BeginExit() {
    SetButtonHighlights(kButtonNone);
    MetScreenMultiSoundBank::BeginExit();
}

// 0x002069d0
void MetConfigControllerScreen::OnEnterFinished() {
    ClearDuplicateAssignment(mRows->mSelected);
    UpdateButtonHighlight();
}

// 0x00206a08
void MetConfigControllerScreen::SetButtonHighlights(int nButton) {
    for (int i = 0; i < kButtonCount; ++i) {
        mButtonMeshes[i]->SetShowing(i == nButton);
    }
}

// 0x00206aa0
int MetConfigControllerScreen::StoreConfig() {
    ControllerConfig &config = GlobalSettings::shared()->mControllers[mControllerIndex];
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        config.SetButton(nRow, ButtonIndexForText(mRowValueTexts[nRow]->mPreWrapText));
    }
    return 1;
}

// 0x00206b30
int MetConfigControllerScreen::ButtonIndexForText(const HxStr &text) const {
    if (text == g_abLeftStickName) {
        return kButtonLeftStick;
    }
    if (text == g_abRightStickName) {
        return kButtonRightStick;
    }
    const int nButton = text[0] - mFirstButtonCode;
    return static_cast<unsigned>(nButton) < kButtonCount ? nButton : kButtonNone;
}

// 0x00206bb0
void MetConfigControllerScreen::OnMsgScreenDismissed(const HxStr &name,
                                                     [[maybe_unused]] int nChoice) {
    if (name == kMissingValuesDialogue) {
        ActivateNamedPanel(HxStr(kThisScreen));
    } else if (name == kNoMemcardDialogue) {
        mExitChoice = kExitCancelled;
        BeginExit();
    }
}

// 0x00206ca8
bool MetConfigControllerScreen::AllRowsAssigned() const {
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        if (ButtonIndexForText(mRowValueTexts[nRow]->mPreWrapText) == kButtonNone) {
            return false;
        }
    }
    return true;
}

// 0x00206d88
char MetConfigControllerScreen::PreviousButtonCode(int nRow, char code) const {
    switch (nRow) {
    case kRowLeftNote1:
    case kRowCenterNote1:
    case kRowRightNote1:
        if (code == kCodeUnassigned || code == kCodeL1) {
            return kCodeR2;
        }
        return code - 1;

    case kRowLeftNote2:
    case kRowCenterNote2:
    case kRowRightNote2:
        if (code == kCodeUnassigned || code == kCodeSquare) {
            return kCodeCross;
        }
        return code - 1;

    case kRowEraseAndPower:
        if (code == kCodeUnassigned || code == kCodeSquare) {
            return kCodeR2;
        }
        return code - 1;

    default:
        return 0;
    }
}

// 0x00206e30
char MetConfigControllerScreen::NextButtonCode(int nRow, char code) const {
    // Yes, the binary moves an unassigned row to the last code of its range in both directions.
    switch (nRow) {
    case kRowLeftNote1:
    case kRowCenterNote1:
    case kRowRightNote1:
        if (code == kCodeUnassigned) {
            return kCodeR2;
        }
        if (code == kCodeR2) {
            return kCodeL1;
        }
        return code + 1;

    case kRowLeftNote2:
    case kRowCenterNote2:
    case kRowRightNote2:
        if (code == kCodeUnassigned) {
            return kCodeCross;
        }
        if (code == kCodeCross) {
            return kCodeSquare;
        }
        return code + 1;

    case kRowEraseAndPower:
        if (code == kCodeUnassigned) {
            return kCodeR2;
        }
        if (code == kCodeR2) {
            return kCodeSquare;
        }
        return code + 1;

    default:
        return 0;
    }
}
