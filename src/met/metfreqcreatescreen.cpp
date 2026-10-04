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
#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/mat.h"

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
#ifdef VIDEO_STANDARD_PAL
static const char *const kLoadNewFreqScreen = "MetLoadNewFreqScreen";
#endif
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

// What MetScreen::mExitChoice records for slot 36 to act on.
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

} // namespace

MetFreqCreateScreen::MetFreqCreateScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mSelectedIdentity(0) {
    mButtonList = new MetButtonList;
    MetFreqMakerAssetManager::shared()->WaitForLoad();
    // Yes, the binary polls once more after the wait and discards the result.
    MetFreqMakerAssetManager::shared()->PollLoad();
    mBurnTexture = FreqAppearance::FindPersonaBurnTexture(kBurnTextureIndex);
}

MetFreqCreateScreen::~MetFreqCreateScreen() {
    delete mButtonList;
}

MetFreqCreateScreen *MetFreqCreateScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetFreqCreateScreen(pRenderer, nPriority);
}

void MetFreqCreateScreen::EnterAndShow() {
    mIdentities = MetFreqMakerAssetManager::shared()->GetIdentityList();
    mButtonList->SetSelected(kPrefabButtonIndex);
    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
    RefreshSelection();
    MetHelpScreen::SelectPreset(MetText(kMetStrHStandardTitle, kStandardTitlePreset));
    {
        HxStr title = MetConfigText(kMetStrTCreateNewChar, kTitleConfigCode, kTitleKey);
        MetScreenTitleScreen::SetTitle(title);
    }
    PushNamedScreen(HxStr(kLeftGizmoScreen));
    MetScreen::EnterAndShow();
}

void MetFreqCreateScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mButtonList->SelectPrevious();
        MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandNext:
        mButtonList->SelectNext();
        MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
        break;

    case kMetScreenCommandLeft:
        if (mButtonList->mSelected != kPrefabButtonIndex) {
            return;
        }
        StartRepeatingSound(
            mRenderer->mAnimationFrame, kAlternateInterval, mLeftArrow, kAlternateCycles);
        StepSelection(pCommand);
        break;

    case kMetScreenCommandRight:
        if (mButtonList->mSelected != kPrefabButtonIndex) {
            return;
        }
        StartRepeatingSound(
            mRenderer->mAnimationFrame, kAlternateInterval, mRightArrow, kAlternateCycles);
        StepSelection(pCommand);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kAlternateInterval,
                            mButtonList->mSelectedButton,
                            kAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

void MetFreqCreateScreen::PlayCycleLeftSound(int nSelector) {
    if (mButtonList->mSelected == kPrefabButtonIndex) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

void MetFreqCreateScreen::PlayCycleRightSound(int nSelector) {
    if (mButtonList->mSelected == kPrefabButtonIndex) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}

void MetFreqCreateScreen::OnRepeatingSoundFinished(Rnd::Button *pButton) {
    if (pButton == mLeftArrow || pButton == mRightArrow) {
        return;
    }
    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    mExitChoice = kExitToButtonAction;
    BeginExit();
}

void MetFreqCreateScreen::OnExitFinished() {
    if (mExitChoice == kExitBack) {
#ifdef VIDEO_STANDARD_PAL
        if (MetPersonaData::loadList()->size() != 0) {
            PushNamedScreen(HxStr(kLoadFreqScreen));
            ActivateNamedPanel(HxStr(kLoadFreqScreen));
        } else {
            PushNamedScreen(HxStr(kLoadNewFreqScreen));
            ActivateNamedPanel(HxStr(kLoadNewFreqScreen));
        }
#else
        PushNamedScreen(HxStr(kLoadFreqScreen));
        ActivateNamedPanel(HxStr(kLoadFreqScreen));
#endif
    } else {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kFreqCreateScreen);
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

void MetFreqCreateScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mLeftArrow = dynamic_cast<Rnd::Button *>(Rnd::TheManager.Find(HxStr(kLeftArrowObject)));
    mRightArrow = dynamic_cast<Rnd::Button *>(Rnd::TheManager.Find(HxStr(kRightArrowObject)));
    mLeftArrow->SetState(kArrowShownState);
    mRightArrow->SetState(kArrowShownState);
    mButtonList->Add(HxStr(kPrefabButtonObject),
                     MetConfigText(kMetStrCreateFromPrefab, kPromptConfigCode, kPrefabPrompt));
    mButtonList->Add(HxStr(kCreateButtonObject),
                     MetConfigText(kMetStrCreateFromScratch, kPromptConfigCode, kCreatePrompt));
    mHelpKeys.clear();
    mHelpKeys.push_back(MetText(kMetStrHCreateFromPrefab, kPrefabPrompt));
    mHelpKeys.push_back(MetText(kMetStrHCreateFromScratch, kCreatePrompt));
}

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

void MetFreqCreateScreen::RefreshSelection() {
    Rnd::Mat *pMat = dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr(kPreviewMaterial)));
    (*mIdentities)[mSelectedIdentity]->AttachToBurnSlot(kPreviewBurnSlot);
    pMat->mStages[kPreviewStage].SetTex(mBurnTexture);
}
