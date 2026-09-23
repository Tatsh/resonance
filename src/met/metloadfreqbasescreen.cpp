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

// Entries the identity list needs before either cycle sound plays.
constexpr unsigned kMinimumCyclableEntries = 2;

// MetButtonList::mSelected while the identity carousel rather than a button is selected.
constexpr int kCarouselSelected = 0;

// Index of the first button in the ring, which BuildButtonList() selects and UpdateNameLabel()
// writes the identity username into.
constexpr int kNameButtonIndex = 0;
constexpr int kEditButtonIndex = 1;
constexpr int kCreateButtonIndex = 2;

// What MetScreen::mUnknown18 records for the exit hook to act on. The back command writes the
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

MetLoadFreqBaseScreen::MetLoadFreqBaseScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
    mUnknown94 = 0;
    mUnknown90 = new MetButtonList;
    MetFreqMakerAssetManager::shared()->WaitForLoad();
    // Yes, the binary polls once more after the wait has already run the load to completion, and
    // discards the result.
    MetFreqMakerAssetManager::shared()->PollLoad();
    mBurnTexture = FreqAppearance::FindPersonaBurnTexture(kBurnTextureIndex);
}

MetLoadFreqBaseScreen::~MetLoadFreqBaseScreen() {
    delete mUnknown90;
}

void MetLoadFreqBaseScreen::EnterAndShow() {
    AcquireIdentityList();
    BuildButtonList();
    UpdateCycleArrows();

    if (static_cast<unsigned>(mUnknown94) >= mUnknown8c->size()) {
        mUnknown94 = 0;
    }

    MetHelpScreen::SetText(mUnknown38[mUnknown90->mSelected], mUnknown10->mUnknown68);
    RefreshSelection();
    PushNamedScreen(HxStr(kLeftGizmoScreen));
    MetScreen::EnterAndShow();
}

void MetLoadFreqBaseScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandPrevious:
        mUnknown90->OnUnknownSlot2();
        MetHelpScreen::SetText(mUnknown38[mUnknown90->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandNext:
        mUnknown90->OnUnknownSlot3();
        MetHelpScreen::SetText(mUnknown38[mUnknown90->mSelected], mUnknown10->mUnknown68);
        break;

    case kMetScreenCommandLeft:
        if (mUnknown90->mSelected != kCarouselSelected) {
            return;
        }
        StartRepeatingSound(
            mUnknown10->mUnknown68, kArrowAlternateInterval, mUnknown98, kArrowAlternateCycles);
        StepSelection(pCommand);
        break;

    case kMetScreenCommandRight:
        if (mUnknown90->mSelected != kCarouselSelected) {
            return;
        }
        StartRepeatingSound(
            mUnknown10->mUnknown68, kArrowAlternateInterval, mUnknown9c, kArrowAlternateCycles);
        StepSelection(pCommand);
        break;

    case kMetScreenCommandSelect:
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        StartRepeatingSound(mUnknown10->mUnknown68,
                            kArrowAlternateInterval,
                            mUnknown90->mUnknown00,
                            kArrowAlternateCycles);
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mUnknown10->mUnknown68);
        mUnknown18 = kExitToMainMenu;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kLeftGizmoScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

void MetLoadFreqBaseScreen::PlayCycleLeftSound(int nSelector) {
    if (mUnknown90->mSelected == kCarouselSelected &&
        mUnknown8c->size() >= kMinimumCyclableEntries) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

void MetLoadFreqBaseScreen::PlayCycleRightSound(int nSelector) {
    if (mUnknown90->mSelected == kCarouselSelected &&
        mUnknown8c->size() >= kMinimumCyclableEntries) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}

void MetLoadFreqBaseScreen::OnUnknownSlot30(Rnd::Object *pObject) {
    if (pObject == mUnknown98 || pObject == mUnknown9c) {
        return;
    }

    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    mUnknown18 = kExitToButtonAction;
    BeginExit();
}

void MetLoadFreqBaseScreen::OnUnknownSlot36() {
    if (mUnknown18 == kExitToMainMenu) {
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
        return;
    }

    switch (mUnknown90->mSelected) {
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

    mUnknown90->SetSelected(-1);
}

void MetLoadFreqBaseScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    Rnd::Object *pLeft = Rnd::g_manager.Find(HxStr(kLeftArrowObject));
    mUnknown98 = pLeft != nullptr ? dynamic_cast<Rnd::Button *>(pLeft) : nullptr;

    Rnd::Object *pRight = Rnd::g_manager.Find(HxStr(kRightArrowObject));
    mUnknown9c = pRight != nullptr ? dynamic_cast<Rnd::Button *>(pRight) : nullptr;
}

void MetLoadFreqBaseScreen::UpdateNameLabel() {
    HxStr username((*mUnknown8c)[mUnknown94]->mUnknown140.mUnknown00);
    mUnknown90->ButtonAt(kNameButtonIndex)->mText->SetText(username);
}

void MetLoadFreqBaseScreen::OnNameButton() {
}

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
    pCanvas->LoadPrefab((*mUnknown8c)[mUnknown94], kNoRandomize);
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

void MetLoadFreqBaseScreen::AcquireIdentityList() {
}

void MetLoadFreqBaseScreen::BuildButtonList() {
    mUnknown90->Clear();
    mUnknown90->Add(HxStr(kNameButtonObject), HxStr(kNoName));
    mUnknown90->Add(HxStr(kEditButtonObject), HxStr(kNoName));
    mUnknown90->Add(HxStr(kCreateButtonObject), HxStr(kNoName));

    mUnknown38.erase(mUnknown38.begin(), mUnknown38.end());
    mUnknown38.push_back(HxStr(kNamePrompt));
    mUnknown38.push_back(HxStr(kEditPrompt));
    mUnknown38.push_back(HxStr(kCreatePrompt));

    mUnknown90->SetSelected(kNameButtonIndex);
}

void MetLoadFreqBaseScreen::UpdateCycleArrows() {
    const int nShowing = mUnknown8c->size() >= kMinimumCyclableEntries ? 1 : 0;

    mUnknown98->SetShowing(nShowing);
    mUnknown9c->SetShowing(nShowing);

    if (nShowing != 0) {
        mUnknown98->SetState(kArrowShownState);
        mUnknown9c->SetState(kArrowShownState);
    }
}

void MetLoadFreqBaseScreen::StepSelection(const MetScreenCommand *pCommand) {
    if (pCommand->mCommand == kMetScreenCommandLeft) {
        int nIndex = mUnknown94 - 1;
        if (nIndex < 0) {
            nIndex = static_cast<int>(mUnknown8c->size()) - 1;
        }
        mUnknown94 = nIndex;
    } else {
        int nIndex = mUnknown94 + 1;
        if (nIndex >= static_cast<int>(mUnknown8c->size())) {
            nIndex = 0;
        }
        mUnknown94 = nIndex;
    }

    RefreshSelection();
}
