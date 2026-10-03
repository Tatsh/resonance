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
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/text.h"

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

// The European release's identifiers of each row's label and of its help text.
const MetStringId kRowLabelIds[] = {
    kMetStrControllerConfigLeftNote1,
    kMetStrControllerConfigLeftNote2,
    kMetStrControllerConfigCenterNote1,
    kMetStrControllerConfigCenterNote2,
    kMetStrControllerConfigRightNote1,
    kMetStrControllerConfigRightNote2,
    kMetStrControllerConfigEraseAndPower,
    kMetStrControllerConfigExpression,
    kMetStrControllerConfigRemixFx,
};

const MetStringId kRowHelpIds[] = {
    kMetStrHControllerConfigLeftNote1,
    kMetStrHControllerConfigLeftNote2,
    kMetStrHControllerConfigCenterNote1,
    kMetStrHControllerConfigCenterNote2,
    kMetStrHControllerConfigRightNote1,
    kMetStrHControllerConfigRightNote2,
    kMetStrHControllerConfigEraseAndPower,
    kMetStrHControllerConfigExpression,
    kMetStrHControllerConfigRemixFx,
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

#ifdef VIDEO_STANDARD_PAL
// A text object the European release fills from its text table.
struct LabelText {
    const char *pszObject;
    MetStringId nId;
};

// The labels filled ahead of the instructions.
const LabelText kActionLabels[] = {
    {"psx_rotate.txt", kMetStrControllerConfigRotate},
    {"psx_advance.txt", kMetStrControllerConfigAdvance},
    {"psx_loopmode.txt", kMetStrControllerConfigLoop},
    {"psx_exit.txt", kMetStrControllerConfigExit},
    {"psx_playback.txt", kMetStrControllerConfigPlayback},
};

// The labels filled after the instructions. The last two are the stick rows' values.
const LabelText kPanelLabels[] = {
    {"psx_actions_pan.txt", kMetStrPsxActionsPan},
    {"psx_diagram_pan.txt", kMetStrPsxDiagramPan},
    {"psx_express_val.txt", kMetStrPsxLeftAnalog},
    {"psx_remixfx_val.txt", kMetStrPsxRightAnalog},
};

inline void FillLabel(const LabelText &label) {
    dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(label.pszObject)))
        ->SetText(GetMetString(label.nId));
}
#endif

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

#ifdef VIDEO_STANDARD_PAL
// The European release reads the stick labels from its text table.
inline HxStr LeftStickName() {
    return GetMetString(kMetStrPsxLeftAnalog);
}

inline HxStr RightStickName() {
    return GetMetString(kMetStrPsxRightAnalog);
}
#else
// NTSC-U/C: 0x00891a90
HxStr g_abLeftStickName("left analog stick");
// NTSC-U/C: 0x00891a98
HxStr g_abRightStickName("right analog stick");

inline const HxStr &LeftStickName() {
    return g_abLeftStickName;
}

inline const HxStr &RightStickName() {
    return g_abRightStickName;
}
#endif

// NTSC-U/C: 0x00891aa0, PAL: 0x008d61b0
// The value of a row with no button.
HxStr g_abUnassignedName("o");

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

inline bool IsStickRow(int nRow) {
    return nRow == kRowExpression || nRow == kRowRemixFX;
}

// Swaps a stick row between the two sticks.
inline void ToggleStick(Rnd::Text *pText) {
    if (pText->mPreWrapText == LeftStickName()) {
        pText->SetText(RightStickName());
    } else {
        pText->SetText(LeftStickName());
    }
}

} // namespace

// NTSC-U/C: 0x001ff1e0, PAL: 0x002066c8
MetConfigControllerScreen::MetConfigControllerScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreenMultiSoundBank(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mRows(nullptr), mFirstButtonCode(kCodeSquare), mLastButtonCode(kCodeR2), mControllerIndex(0) {
    mRows = new MetButtonList();

    // The binary expands each loop in this constructor into one call per entry.
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mHelpKeys.push_back(MetText(kRowHelpIds[nRow], kRowKeys[nRow]));
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

// NTSC-U/C: 0x002008e8, PAL: 0x00208cb0
MetConfigControllerScreen::~MetConfigControllerScreen() {
    delete mRows;
}

// NTSC-U/C: 0x001fff40, PAL: 0x00207618
void MetConfigControllerScreen::ResolveContainerViews() {
    MetScreenMultiSoundBank::ResolveContainerViews();

    // The binary expands this loop into one call per row.
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mRows->Add(HxStr(kRowButtons[nRow]),
                   MetConfigText(kRowLabelIds[nRow], kPromptConfigCode, kRowKeys[nRow]));
    }

#ifdef VIDEO_STANDARD_PAL
    // The binary expands this loop into one call per label.
    for (const auto &label : kActionLabels) {
        FillLabel(label);
    }
#endif

    // Yes, the binary does not test either instruction text for null.
    dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kInstructionText1)))
        ->SetText(
            MetConfigText(kMetStrControllerConfigInstruct1, kPromptConfigCode, kInstructionKey1));
    dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr(kInstructionText2)))
        ->SetText(
            MetConfigText(kMetStrControllerConfigInstruct2, kPromptConfigCode, kInstructionKey2));

#ifdef VIDEO_STANDARD_PAL
    // The binary expands this loop into one call per label.
    for (const auto &label : kPanelLabels) {
        FillLabel(label);
    }
#endif

    for (int nButton = 0; nButton < kButtonCount; ++nButton) {
        mButtonMeshes[nButton] =
            dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(mButtonMeshNames[nButton]));
        mButtonMeshes[nButton]->SetShowing(false);
    }

    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        mRowValueTexts[nRow] =
            dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(mRowValueTextNames[nRow]));
    }
}

// NTSC-U/C: 0x00200ba8, PAL: 0x00208f98
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
            buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
            MetMsgScreen::Show(HxStr(kMissingValuesDialogue),
                               MetText(kMetStrMsgERROR, kErrorTitle),
                               MetText(kMetStrControllerConfigWarn, kMissingValuesText),
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

// NTSC-U/C: 0x00201478, PAL: 0x00209bd0
void MetConfigControllerScreen::EnterAndShow() {
    mRows->SetSelected(0);

    HxStr title;
    HxStr options;
    HxStr player;
    player = MetConfigText(kMetStrConfigControllerPlayer, kPromptConfigCode, kPlayerKey);
    options = MetConfigText(kMetStrTConfigControllerOptions, kTitleConfigCode, kOptionsKey);
    title =
        FormatString(kTitleFormat, TextOrEmpty(player), mControllerIndex + 1, TextOrEmpty(options));
    MetScreenTitleScreen::SetTitle(title);

    MetHelpScreen::SetText(mHelpKeys[mRows->mSelected], mRenderer->mAnimationFrame);
#ifdef VIDEO_STANDARD_PAL
    // The European release offers to save unless the screen was opened from a pause screen.
    if (MetFrontEndState::shared()->mReturnScreen == kPauseGameScreen ||
        MetFrontEndState::shared()->mReturnScreen == kPauseRemixScreen) {
        MetHelpScreen::SelectPreset(GetMetString(kMetStrHStandardTitle));
    } else {
        MetHelpScreen::SelectPreset(GetMetString(kMetStrHCcSaveBack));
    }
#else
    if (MetFrontEndState::shared()->mUsingMemcard != 0) {
        MetHelpScreen::SelectPreset(HxStr(kSaveBackPreset));
    } else {
        MetHelpScreen::SelectPreset(HxStr(kStandardPreset));
    }
#endif

    ShowConfig(GlobalSettings::shared()->mControllers[mControllerIndex]);
    MetScreenMultiSoundBank::EnterAndShow();
}

// NTSC-U/C: 0x00201790, PAL: 0x00209fa0
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

// NTSC-U/C: 0x00201eb8, PAL: 0x0020a818
void MetConfigControllerScreen::ClearDuplicateAssignment(int nRow) {
    if (IsStickRow(nRow)) {
        Rnd::Text *pOther = mRowValueTexts[nRow == kRowExpression ? kRowRemixFX : kRowExpression];
        const HxStr &value = mRowValueTexts[nRow]->mPreWrapText;
        if (value == LeftStickName()) {
            pOther->SetText(RightStickName());
        } else if (value == RightStickName()) {
            pOther->SetText(LeftStickName());
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

// NTSC-U/C: 0x00202070, PAL: 0x0020ab70
void MetConfigControllerScreen::UpdateButtonHighlight() {
    const int nRow = mRows->mSelected;
    HxStr value(mRowValueTexts[nRow]->mPreWrapText);
    if (IsStickRow(nRow)) {
        SetButtonHighlights(value == LeftStickName() ? kButtonLeftStick : kButtonRightStick);
        return;
    }

    const int nButton = value[0] - mFirstButtonCode;
    SetButtonHighlights(static_cast<unsigned>(nButton) < kButtonCount ? nButton : kButtonNone);
}

// NTSC-U/C: 0x002022a0, PAL: 0x0020afa8
void MetConfigControllerScreen::ShowConfig(ControllerConfig &config) {
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        const int nButton = config.GetButtonIndex(nRow);
        Rnd::Text *pText = mRowValueTexts[nRow];
        if (nButton == kButtonLeftStick) {
            pText->SetText(LeftStickName());
        } else if (nButton == kButtonRightStick) {
            pText->SetText(RightStickName());
        } else {
            pText->SetText(HxStr(1, static_cast<char>(mFirstButtonCode + nButton)));
        }
    }
}

// NTSC-U/C: 0x00206828, PAL: 0x0020f6f0
MetConfigControllerScreen *MetConfigControllerScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetConfigControllerScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x002068b0, PAL: 0x0020f778
void MetConfigControllerScreen::PlaySlideSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlaySlideSound(nSelector);
    }
}

// NTSC-U/C: 0x002068e0, PAL: 0x0020f7a8
void MetConfigControllerScreen::PlayCycleLeftSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlayCycleLeftSound(nSelector);
    }
}

// NTSC-U/C: 0x00206910, PAL: 0x0020f7d8
void MetConfigControllerScreen::PlayCycleRightSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlayCycleRightSound(nSelector);
    }
}

// NTSC-U/C: 0x00206940, PAL: 0x0020f808
void MetConfigControllerScreen::PlayHighSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlayHighSound(nSelector);
    }
}

// NTSC-U/C: 0x00206970, PAL: 0x0020f838
void MetConfigControllerScreen::PlayLeaveSound(int nSelector) {
    if (nSelector == mControllerIndex + 1) {
        MetScreenMultiSoundBank::PlayLeaveSound(nSelector);
    }
}

// NTSC-U/C: 0x002069a0, PAL: 0x0020f868
void MetConfigControllerScreen::BeginExit() {
    SetButtonHighlights(kButtonNone);
    MetScreenMultiSoundBank::BeginExit();
}

// NTSC-U/C: 0x002069d0, PAL: 0x0020f898
void MetConfigControllerScreen::OnEnterFinished() {
    ClearDuplicateAssignment(mRows->mSelected);
    UpdateButtonHighlight();
}

// NTSC-U/C: 0x00206a08, PAL: 0x0020f8d0
void MetConfigControllerScreen::SetButtonHighlights(int nButton) {
    for (int i = 0; i < kButtonCount; ++i) {
        mButtonMeshes[i]->SetShowing(i == nButton);
    }
}

// NTSC-U/C: 0x00206aa0, PAL: 0x0020f968
int MetConfigControllerScreen::StoreConfig() {
    ControllerConfig &config = GlobalSettings::shared()->mControllers[mControllerIndex];
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        config.SetButton(nRow, ButtonIndexForText(mRowValueTexts[nRow]->mPreWrapText));
    }
    return 1;
}

// NTSC-U/C: 0x00206b30, PAL: 0x0020ae38
int MetConfigControllerScreen::ButtonIndexForText(const HxStr &text) const {
    if (text == LeftStickName()) {
        return kButtonLeftStick;
    }
    if (text == RightStickName()) {
        return kButtonRightStick;
    }
    const int nButton = text[0] - mFirstButtonCode;
    return static_cast<unsigned>(nButton) < kButtonCount ? nButton : kButtonNone;
}

// NTSC-U/C: 0x00206bb0, PAL: 0x0020f9f8
void MetConfigControllerScreen::OnMsgScreenDismissed(const HxStr &name,
                                                     [[maybe_unused]] int nChoice) {
    if (name == kMissingValuesDialogue) {
        ActivateNamedPanel(HxStr(kThisScreen));
    } else if (name == kNoMemcardDialogue) {
        mExitChoice = kExitCancelled;
        BeginExit();
    }
}

// NTSC-U/C: 0x00206ca8, PAL: 0x0020fb10
bool MetConfigControllerScreen::AllRowsAssigned() const {
    for (int nRow = 0; nRow < kRowCount; ++nRow) {
        if (ButtonIndexForText(mRowValueTexts[nRow]->mPreWrapText) == kButtonNone) {
            return false;
        }
    }
    return true;
}

// NTSC-U/C: 0x00206d88, PAL: 0x0020fb88
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

// NTSC-U/C: 0x00206e30, PAL: 0x0020fc30
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
