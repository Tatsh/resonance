#include "met/metfreqcreatescreen.h"

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "met/metbuttonlist.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakerbuttonsscreen.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "script/configquery.h"

namespace {

static const char *const kScreenName = "cf";
static const char *const kDirectory = "metagame/_Solo";
static const char *const kContainerName = "create_freq";

// The two arrows and the two buttons, with the prompts the buttons are labelled from.
static const char *const kLeftArrowObject = "cf_left.but";
static const char *const kRightArrowObject = "cf_right.but";
static const char *const kPrefabButtonObject = "cf_prefab.but";
static const char *const kCreateButtonObject = "cf_create.but";
static const char *const kPrefabPrompt = "create_from_prefab";
static const char *const kCreatePrompt = "create_from_scratch";

// The material the selected identity is burned into.
static const char *const kPreviewMaterial = "cf_char.mat";

// The help preset and the title key EnterAndShow() uses.
static const char *const kStandardTitlePreset = "standard_title";
static const char *const kTitleKey = "create_new_char";

static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kLoadFreqScreen = "MetLoadFreqScreen";
static const char *const kFreqCreateScreen = "MetFreqCreateScreen";
static const char *const kFreqMakerCanvasScreen = "MetFreqMakerCanvasScreen";
static const char *const kFreqMakerButtonsScreen = "MetFreqMakerButtonsScreen";
static const char *const kFreqMakerDirectionsScreen = "MetFreqMakerDirectionsScreen";
static const char *const kFreqMakerInventoryScreen = "MetFreqMakerInventoryScreen";
static const char *const kNoName = "";

// Configuration codes the button labels and the screen title are read under.
constexpr int kPromptConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// The burn texture the constructor resolves, the burn slot the preview uses, and the material
// stage that shows it.
constexpr int kBurnTextureIndex = 0;
constexpr int kPreviewBurnSlot = 0;
constexpr int kPreviewStage = 1;

// The button ring indices.
constexpr int kPrefabButtonIndex = 0;
constexpr int kCreateButtonIndex = 1;
constexpr int kNoSelection = -1;

// What MetScreen::mUnknown18 records for slot 36 to act on.
constexpr int kExitBack = 0;
constexpr int kExitToButtonAction = 2;

// The state the two arrows take, and the alternation each command starts.
constexpr int kArrowShownState = 1;
constexpr float kAlternateInterval = 30.0f;
constexpr int kAlternateCycles = 2;

// The MetFreqMakerButtonsScreen::SetEditing() values, and the LoadPrefab() randomise flag.
constexpr int kFreqMakerCreating = 0;
constexpr int kFreqMakerEditing = 1;
constexpr int kNoRandomize = 0;

// A dialogue text read by value from configuration.
inline HxStr ConfigText(int nCode, const char *pszKey) {
    HxStr text;
    QueryConfigString(&text, nCode, pszKey);
    return text;
}

} // namespace

// 0x0029c130
MetFreqCreateScreen::MetFreqCreateScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mSelectedIdentity = 0;
    mButtonList = new MetButtonList;
    MetFreqMakerAssetManager::shared()->WaitForLoad();
    // Yes, the binary polls once more after the wait and discards the result.
    MetFreqMakerAssetManager::shared()->PollLoad();
    mBurnTexture = FreqAppearance::FindPersonaBurnTexture(kBurnTextureIndex);
}

// 0x002a0b08
MetFreqCreateScreen::~MetFreqCreateScreen() {
    delete mButtonList;
}

// 0x002a0a80
MetFreqCreateScreen *MetFreqCreateScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetFreqCreateScreen(pRenderer, nPriority);
}

// 0x0029ccb8
void MetFreqCreateScreen::EnterAndShow() {
    mIdentities = MetFreqMakerAssetManager::shared()->GetIdentityList();
    mButtonList->SetSelected(kPrefabButtonIndex);
    MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
    RefreshSelection();
    MetHelpScreen::SelectPreset(HxStr(kStandardTitlePreset));
    {
        HxStr title;
        QueryConfigString(&title, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    }
    PushNamedScreen(HxStr(kLeftGizmoScreen));
    MetScreen::EnterAndShow();
}

// 0x0029c7a0
void MetFreqCreateScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtonList->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mButtonList->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mButtonList->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandLeft:
        if (mButtonList->mSelected != kPrefabButtonIndex) {
            return;
        }
        StartRepeatingSound(
            mUnknown10->mUnknown68, kAlternateInterval, mLeftArrow, kAlternateCycles);
        StepSelection(pCommand);
        break;

    case kMetScreenCommandRight:
        if (mButtonList->mSelected != kPrefabButtonIndex) {
            return;
        }
        StartRepeatingSound(
            mUnknown10->mUnknown68, kAlternateInterval, mRightArrow, kAlternateCycles);
        StepSelection(pCommand);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        StartRepeatingSound(
            mUnknown10->mUnknown68, kAlternateInterval, mButtonList->mUnknown00, kAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        mUnknown18 = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x002a0b88
void MetFreqCreateScreen::PlayCycleLeftSound(int nSelector) {
    if (mButtonList->mSelected == kPrefabButtonIndex) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

// 0x002a0bb8
void MetFreqCreateScreen::PlayCycleRightSound(int nSelector) {
    if (mButtonList->mSelected == kPrefabButtonIndex) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}

// 0x0029ce58
void MetFreqCreateScreen::OnUnknownSlot30(Rnd::Button *pButton) {
    if (pButton == mLeftArrow || pButton == mRightArrow) {
        return;
    }
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    mUnknown18 = kExitToButtonAction;
    BeginExit();
}

// 0x0029cfa0
void MetFreqCreateScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitBack) {
        PushNamedScreen(HxStr(kLoadFreqScreen));
        ActivateNamedPanel(HxStr(kLoadFreqScreen));
    } else {
        MetFrontEndState::shared()->mUnknown24 = HxStr(kFreqCreateScreen);
        MetFreqMakerCanvasScreen *pCanvas = static_cast<MetFreqMakerCanvasScreen *>(
            FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
        MetFreqMakerButtonsScreen *pButtons = static_cast<MetFreqMakerButtonsScreen *>(
            FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
        pButtons->mNewPersona = 1;
        switch (mButtonList->mSelected) {
        case kPrefabButtonIndex:
            pCanvas->LoadPrefab((*mIdentities)[mSelectedIdentity], kNoRandomize);
            pButtons->SetEditing(kFreqMakerEditing);
            break;
        case kCreateButtonIndex:
            pCanvas->LoadPersona(nullptr);
            pButtons->SetEditing(kFreqMakerCreating);
            break;
        default:
            break;
        }
        Application::shared()->GetGameManager()->ClearPersonas();
        PushNamedScreen(HxStr(kFreqMakerCanvasScreen));
        PushNamedScreen(HxStr(kFreqMakerDirectionsScreen));
        PushNamedScreen(HxStr(kFreqMakerInventoryScreen));
        PushNamedScreen(HxStr(kFreqMakerButtonsScreen));
        ActivateNamedPanel(HxStr(kFreqMakerButtonsScreen));
    }
    mButtonList->SetSelected(kNoSelection);
}

// 0x0029c330
void MetFreqCreateScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mLeftArrow = dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(kLeftArrowObject)));
    mRightArrow = dynamic_cast<Rnd::Button *>(Rnd::g_manager.Find(HxStr(kRightArrowObject)));
    mLeftArrow->SetState(kArrowShownState);
    mRightArrow->SetState(kArrowShownState);
    mButtonList->Add(HxStr(kPrefabButtonObject), ConfigText(kPromptConfigCode, kPrefabPrompt));
    mButtonList->Add(HxStr(kCreateButtonObject), ConfigText(kPromptConfigCode, kCreatePrompt));
    mUnknown38.clear();
    mUnknown38.push_back(HxStr(kPrefabPrompt));
    mUnknown38.push_back(HxStr(kCreatePrompt));
}

// 0x0029cb30
void MetFreqCreateScreen::StepSelection(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandLeft) {
        int nIndex = mSelectedIdentity - 1;
        if (nIndex < 0) {
            nIndex = static_cast<int>(mIdentities->size()) - 1;
        }
        mSelectedIdentity = nIndex;
    } else {
        int nIndex = mSelectedIdentity + 1;
        if (nIndex >= static_cast<int>(mIdentities->size())) {
            nIndex = 0;
        }
        mSelectedIdentity = nIndex;
    }
    RefreshSelection();
}

// 0x0029cbb8
void MetFreqCreateScreen::RefreshSelection() {
    Rnd::Mat *pMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr(kPreviewMaterial)));
    (*mIdentities)[mSelectedIdentity]->AttachToBurnSlot(kPreviewBurnSlot);
    pMat->mStages[kPreviewStage].SetTex(mBurnTexture);
}
