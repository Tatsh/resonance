#include "met/metloadfreqbasescreen.h"

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakerbuttonsscreen.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/text.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "cid";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "create_id";

// The two cycle arrows ResolveContainerViews() resolves.
static const char *const kLeftArrowObject = "cid_left.but";
static const char *const kRightArrowObject = "cid_right.but";

// The three buttons BuildButtonList() appends, in ring order.
static const char *const kNameButtonObject = "cid_01.but";
static const char *const kEditButtonObject = "cid_02.but";
static const char *const kCreateButtonObject = "cid_03.but";

// The three prompts BuildButtonList() appends, matching the button order above.
static const char *const kNamePrompt = "id_name";
static const char *const kEditPrompt = "cid_edit";
static const char *const kCreatePrompt = "id_create";

// Screens the class pushes and exits by registry key.
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kLeftGizmoSmallScreen = "MetLeftGizmoSmallScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kTopLogoScreen = "MetTopLogoScreen";
static const char *const kMainScreen = "MetMainScreen";
static const char *const kFreqMakerCanvasScreen = "MetFreqMakerCanvasScreen";
static const char *const kFreqMakerButtonsScreen = "MetFreqMakerButtonsScreen";
static const char *const kFreqMakerDirectionsScreen = "MetFreqMakerDirectionsScreen";
static const char *const kFreqMakerInventoryScreen = "MetFreqMakerInventoryScreen";

// The empty literal every call site that clears a prompt or a panel passes.
static const char *const kNoName = "";

// The burn texture the constructor resolves.
constexpr int kBurnTextureIndex = 0;

// The material RefreshSelection() burns the selected identity into, the burn slot, and the
// material stage that shows it.
static const char *const kPreviewMaterial = "cid_char.mat";
constexpr int kPreviewBurnSlot = 0;
constexpr int kPreviewStage = 1;

// Entries the identity list needs before either cycle sound plays.
constexpr unsigned kMinimumCyclableEntries = 2;

// MetButtonList::mSelected while the identity carousel rather than a button is selected.
constexpr int kCarouselSelected = 0;

// Index of the first button in the ring, which BuildButtonList() selects and UpdateNameLabel()
// writes the identity username into.
constexpr int kNameButtonIndex = 0;
constexpr int kEditButtonIndex = 1;
constexpr int kCreateButtonIndex = 2;

// What MetScreen::mExitChoice records for the exit hook to act on. The back command writes the
// first and the alternation-finished hook writes the second.
constexpr int kExitToMainMenu = 0;
constexpr int kExitToButtonAction = 2;

// The MetFreqMakerButtonsScreen::SetEditing() values, and the LoadPrefab() randomise flag.
constexpr int kFreqMakerCreating = 0;
constexpr int kFreqMakerEditing = 1;
constexpr int kNoRandomize = 0;

// Button state the two cycle arrows take while they are shown.
constexpr int kArrowShownState = 1;

// Cycles and interval the two arrows alternate over while a cycle command is held.
constexpr int kArrowAlternateCycles = 2;
constexpr float kArrowAlternateInterval = 30.0f;

} // namespace

// 0x00291e00
MetLoadFreqBaseScreen::MetLoadFreqBaseScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mSelectedIdentity = 0;
    mButtonList = new MetButtonList;
    MetFreqMakerAssetManager::shared()->WaitForLoad();
    // Yes, the binary polls once more after the wait has already run the load to completion, and
    // discards the result.
    MetFreqMakerAssetManager::shared()->PollLoad();
    mBurnTexture = FreqAppearance::FindPersonaBurnTexture(kBurnTextureIndex);
}

// 0x00296ae0
MetLoadFreqBaseScreen::~MetLoadFreqBaseScreen() {
    delete mButtonList;
}

// 0x00296a58
MetScreen *MetLoadFreqBaseScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLoadFreqBaseScreen(pRenderer, nPriority);
}

// 0x002926d8
void MetLoadFreqBaseScreen::EnterAndShow() {
    AcquireIdentityList();
    BuildButtonList();
    UpdateCycleArrows();

    if (static_cast<unsigned>(mSelectedIdentity) >= mIdentityList->size()) {
        mSelectedIdentity = 0;
    }

    MetHelpScreen::SetText(mHelpKeys[mButtonList->mSelected], mRenderer->mAnimationFrame);
    RefreshSelection();
    PushNamedScreen(HxStr(kLeftGizmoScreen));
    MetScreen::EnterAndShow();
}

// 0x00292178
void MetLoadFreqBaseScreen::HandleCommand(const MetScreenCommand *pCommand) {
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
        if (mButtonList->mSelected != kCarouselSelected) {
            return;
        }
        StartRepeatingSound(
            mRenderer->mAnimationFrame, kArrowAlternateInterval, mLeftArrow, kArrowAlternateCycles);
        StepSelection(pCommand);
        break;

    case kMetScreenCommandRight:
        if (mButtonList->mSelected != kCarouselSelected) {
            return;
        }
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kArrowAlternateInterval,
                            mRightArrow,
                            kArrowAlternateCycles);
        StepSelection(pCommand);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kArrowAlternateInterval,
                            mButtonList->mSelectedButton,
                            kArrowAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitToMainMenu;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x00296b60
void MetLoadFreqBaseScreen::PlayCycleLeftSound(int nSelector) {
    if (mButtonList->mSelected == kCarouselSelected &&
        mIdentityList->size() >= kMinimumCyclableEntries) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

// 0x00296bb0
void MetLoadFreqBaseScreen::PlayCycleRightSound(int nSelector) {
    if (mButtonList->mSelected == kCarouselSelected &&
        mIdentityList->size() >= kMinimumCyclableEntries) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}

// 0x00292c60
void MetLoadFreqBaseScreen::OnRepeatingSoundFinished(Rnd::Button *pButton) {
    if (pButton == mLeftArrow || pButton == mRightArrow) {
        return;
    }

    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    mExitChoice = kExitToButtonAction;
    BeginExit();
}

// 0x00292da8
void MetLoadFreqBaseScreen::OnExitFinished() {
    if (mExitChoice == kExitToMainMenu) {
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
    } else {
        switch (mButtonList->mSelected) {
        case kNameButtonIndex:
            OnNameButton();
            break;
        case kEditButtonIndex:
            OnEditButton();
            break;
        case kCreateButtonIndex:
            OnCreateButton();
            break;
        default:
            break;
        }
    }

    mButtonList->SetSelected(-1);
}

// 0x00292000
void MetLoadFreqBaseScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    Rnd::Object *pLeft = Rnd::g_manager.Find(HxStr(kLeftArrowObject));
    mLeftArrow = pLeft != nullptr ? dynamic_cast<Rnd::Button *>(pLeft) : nullptr;

    Rnd::Object *pRight = Rnd::g_manager.Find(HxStr(kRightArrowObject));
    mRightArrow = pRight != nullptr ? dynamic_cast<Rnd::Button *>(pRight) : nullptr;
}

// 0x00292620
void MetLoadFreqBaseScreen::UpdateNameLabel() {
    HxStr username((*mIdentityList)[mSelectedIdentity]->mAppearance.mUserName);
    mButtonList->ButtonAt(kNameButtonIndex)->mText->SetText(username);
}

// 0x00296a50
void MetLoadFreqBaseScreen::OnNameButton() {
}

// 0x00293148
void MetLoadFreqBaseScreen::OnEditButton() {
    PrepareFreqMakerForSelection();
    PushNamedScreen(HxStr(kFreqMakerButtonsScreen));
    PushNamedScreen(HxStr(kFreqMakerCanvasScreen));
    PushNamedScreen(HxStr(kFreqMakerDirectionsScreen));
    PushNamedScreen(HxStr(kFreqMakerInventoryScreen));
    ActivateNamedPanel(HxStr(kFreqMakerButtonsScreen));
}

// 0x00293028
void MetLoadFreqBaseScreen::PrepareFreqMakerForSelection() {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
    MetFreqMakerButtonsScreen *pButtons =
        static_cast<MetFreqMakerButtonsScreen *>(FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
    pCanvas->LoadPrefab((*mIdentityList)[mSelectedIdentity], kNoRandomize);
    pButtons->SetEditing(kFreqMakerEditing);
    pButtons->mNewPersona = 0;
}

// 0x002933b8
void MetLoadFreqBaseScreen::OnCreateButton() {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
    MetFreqMakerButtonsScreen *pButtons =
        static_cast<MetFreqMakerButtonsScreen *>(FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
    pCanvas->LoadPersona(nullptr);
    pButtons->SetEditing(kFreqMakerCreating);
    pButtons->mNewPersona = 1;
    Application::shared()->GetGameManager()->ClearPersonas();
    PushNamedScreen(HxStr(kFreqMakerCanvasScreen));
    PushNamedScreen(HxStr(kFreqMakerDirectionsScreen));
    PushNamedScreen(HxStr(kFreqMakerInventoryScreen));
    PushNamedScreen(HxStr(kFreqMakerButtonsScreen));
    ActivateNamedPanel(HxStr(kFreqMakerButtonsScreen));
}

// 0x00296d08
void MetLoadFreqBaseScreen::AcquireIdentityList() {
}

// 0x00292810
void MetLoadFreqBaseScreen::BuildButtonList() {
    mButtonList->Clear();
    mButtonList->Add(HxStr(kNameButtonObject), HxStr(kNoName));
    mButtonList->Add(HxStr(kEditButtonObject), HxStr(kNoName));
    mButtonList->Add(HxStr(kCreateButtonObject), HxStr(kNoName));

    mHelpKeys.erase(mHelpKeys.begin(), mHelpKeys.end());
    mHelpKeys.push_back(HxStr(kNamePrompt));
    mHelpKeys.push_back(HxStr(kEditPrompt));
    mHelpKeys.push_back(HxStr(kCreatePrompt));

    mButtonList->SetSelected(kNameButtonIndex);
}

// 0x00296c88
void MetLoadFreqBaseScreen::UpdateCycleArrows() {
    const int nShowing = mIdentityList->size() >= kMinimumCyclableEntries ? 1 : 0;

    mLeftArrow->SetShowing(nShowing);
    mRightArrow->SetShowing(nShowing);

    if (nShowing != 0) {
        mLeftArrow->SetState(kArrowShownState);
        mRightArrow->SetState(kArrowShownState);
    }
}

// 0x00292508
void MetLoadFreqBaseScreen::RefreshSelection() {
    Rnd::Mat *pMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr(kPreviewMaterial)));
    (*mIdentityList)[mSelectedIdentity]->AttachToBurnSlot(kPreviewBurnSlot);
    pMat->mStages[kPreviewStage].SetTex(mBurnTexture);
    UpdateNameLabel();
}

// 0x00296c00
void MetLoadFreqBaseScreen::StepSelection(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandLeft) {
        int nIndex = mSelectedIdentity - 1;
        if (nIndex < 0) {
            nIndex = static_cast<int>(mIdentityList->size()) - 1;
        }
        mSelectedIdentity = nIndex;
    } else {
        int nIndex = mSelectedIdentity + 1;
        if (nIndex >= static_cast<int>(mIdentityList->size())) {
            nIndex = 0;
        }
        mSelectedIdentity = nIndex;
    }

    RefreshSelection();
}
