#include "met/metloadfreqbasescreen.h"

#include "app/application.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfreqmakerbuttonsscreen.h"
#include "met/metfreqmakercanvasscreen.h"
#include "met/methelpscreen.h"
#include "met/metrenderer.h"
#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/text.h"

#ifdef VIDEO_STANDARD_PAL
#include "game/globalsettings.h"
#include "met/metfrontendstate.h"
#include "met/metmsgscreen.h"
#include "met/metscreentitlescreen.h"
#include "os/formatstring.h"
#endif

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

// Index of the first button in the ring. BuildButtonList() selects the button and
// UpdateNameLabel() writes the identity username into it.
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

// Cycles and interval the two arrows alternate over while a cycle button stays pressed.
constexpr int kArrowAlternateCycles = 2;
constexpr float kArrowAlternateInterval = 30.0f;

#ifdef VIDEO_STANDARD_PAL
// MetFrontEndState::mUsingMemcard while a memory card is in use.
constexpr int kUsingMemcard = 1;

// Screens OnCreateButton() and OnMsgScreenDismissed() push or compare by registry key.
static const char *const kLoadFreqScreen = "MetLoadFreqScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kFreqCreateScreen = "MetFreqCreateScreen";

// The dialogues the class raises and OnMsgScreenDismissed() matches.
static const char *const kFreqLimitMessage = "freq_limit";
static const char *const kNoCardsMessage = "mem_no_cards";
static const char *const kDetectMessage = "mem_load";

// Identities MetLoadFreqScreen allows before OnCreateButton() refuses another.
constexpr unsigned kMaxIdentities = 8;

// Button counts MetMsgScreen receives, and the RETRY button both dialogues put first.
constexpr int kNoButtons = 0;
constexpr int kTwoButtons = 2;
constexpr int kChoiceRetry = 0;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Raise `freq_limit` with RETRY and CONTINUE and the given text.
inline void ShowFreqLimit(MetScreen *pOwner, const HxStr &text) {
    std::vector<HxStr> buttons;
    buttons.push_back(GetMetString(kMetStrMsgRETRY));
    buttons.push_back(GetMetString(kMetStrMsgCONTINUE));
    MetMsgScreen::Show(HxStr(kFreqLimitMessage),
                       GetMetString(kMetStrMsgERROR),
                       text,
                       kTwoButtons,
                       buttons,
                       pOwner);
}
#endif

} // namespace

// NTSC-U/C: 0x00291e00, PAL: 0x002add68
MetLoadFreqBaseScreen::MetLoadFreqBaseScreen(MetRenderer *pRenderer, int nPriority)
#ifdef VIDEO_STANDARD_PAL
    : MetMemDetectScreen(
#else
    : MetScreen(
#endif
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mSelectedIdentity(0) {
#ifdef VIDEO_STANDARD_PAL
    mUsingMemcardOnEnter = kUsingMemcard;
#endif
    mButtonList = new MetButtonList;
    MetFreqMakerAssetManager::shared()->WaitForLoad();
    // Yes, the binary polls once more after the wait has already run the load to completion, and
    // discards the result.
    MetFreqMakerAssetManager::shared()->PollLoad();
    mBurnTexture = FreqAppearance::FindPersonaBurnTexture(kBurnTextureIndex);
}

// NTSC-U/C: 0x00296ae0, PAL: 0x002b4a40
MetLoadFreqBaseScreen::~MetLoadFreqBaseScreen() {
    delete mButtonList;
}

// NTSC-U/C: 0x00296a58, PAL: 0x002b49b8
MetScreen *MetLoadFreqBaseScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetLoadFreqBaseScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x002926d8, PAL: 0x002ae7d8
void MetLoadFreqBaseScreen::EnterAndShow() {
#ifdef VIDEO_STANDARD_PAL
    mUsingMemcardOnEnter = MetFrontEndState::shared()->mUsingMemcard;
#endif
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

// NTSC-U/C: 0x00292178, PAL: 0x002ae1a0
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

// NTSC-U/C: 0x00296b60, PAL: 0x002b4ac8
void MetLoadFreqBaseScreen::PlayCycleLeftSound(int nSelector) {
    if (mButtonList->mSelected == kCarouselSelected &&
        mIdentityList->size() >= kMinimumCyclableEntries) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

// NTSC-U/C: 0x00296bb0, PAL: 0x002b4b18
void MetLoadFreqBaseScreen::PlayCycleRightSound(int nSelector) {
    if (mButtonList->mSelected == kCarouselSelected &&
        mIdentityList->size() >= kMinimumCyclableEntries) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}

// NTSC-U/C: 0x00292c60, PAL: 0x002aee98
void MetLoadFreqBaseScreen::OnRepeatingSoundFinished(Rnd::Button *pButton) {
    if (pButton == mLeftArrow || pButton == mRightArrow) {
        return;
    }

    ExitScreenByName(HxStr(kLeftGizmoScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    mExitChoice = kExitToButtonAction;
    BeginExit();
}

// NTSC-U/C: 0x00292da8, PAL: 0x002af020
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

// NTSC-U/C: 0x00292000, PAL: 0x002adfe0
void MetLoadFreqBaseScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();

    Rnd::Object *pLeft = Rnd::TheManager.Find(HxStr(kLeftArrowObject));
    mLeftArrow = pLeft != nullptr ? dynamic_cast<Rnd::Button *>(pLeft) : nullptr;

    Rnd::Object *pRight = Rnd::TheManager.Find(HxStr(kRightArrowObject));
    mRightArrow = pRight != nullptr ? dynamic_cast<Rnd::Button *>(pRight) : nullptr;
}

// NTSC-U/C: 0x00292620, PAL: 0x002ae700
void MetLoadFreqBaseScreen::UpdateNameLabel() {
    HxStr username((*mIdentityList)[mSelectedIdentity]->mAppearance.mUserName);
    mButtonList->GetButton(kNameButtonIndex)->mText->SetText(username);
}

// NTSC-U/C: 0x00296a50, PAL: 0x002b49b0
void MetLoadFreqBaseScreen::OnNameButton() {
}

// NTSC-U/C: 0x00293148, PAL: 0x002af488
void MetLoadFreqBaseScreen::OnEditButton() {
    PrepareFreqMakerForSelection();
    PushNamedScreen(HxStr(kFreqMakerButtonsScreen));
    PushNamedScreen(HxStr(kFreqMakerCanvasScreen));
    PushNamedScreen(HxStr(kFreqMakerDirectionsScreen));
    PushNamedScreen(HxStr(kFreqMakerInventoryScreen));
    ActivateNamedPanel(HxStr(kFreqMakerButtonsScreen));
}

// NTSC-U/C: 0x00293028, PAL: 0x002af320
void MetLoadFreqBaseScreen::PrepareFreqMakerForSelection() {
    MetFreqMakerCanvasScreen *pCanvas =
        static_cast<MetFreqMakerCanvasScreen *>(FindScreenByName(HxStr(kFreqMakerCanvasScreen)));
    MetFreqMakerButtonsScreen *pButtons =
        static_cast<MetFreqMakerButtonsScreen *>(FindScreenByName(HxStr(kFreqMakerButtonsScreen)));
    pCanvas->LoadPrefab((*mIdentityList)[mSelectedIdentity], kNoRandomize);
    pButtons->SetEditing(kFreqMakerEditing);
    pButtons->mNewPersona = 0;
}

#ifdef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x002933b8, PAL: 0x002af798
void MetLoadFreqBaseScreen::OnCreateButton() {
    if (MetFrontEndState::shared()->mReturnScreen == kLoadFreqScreen &&
        mIdentityList->size() >= kMaxIdentities) {
        ExitScreenByName(HxStr(kHelpScreen));
        const HxStr format(GetMetString(kMetStrFreqLimit));
        const HxStr text(
            Rnd::MakeString(TextOrEmpty(format),
                            kMaxIdentities,
                            TextOrEmpty(GlobalSettings::shared()->mCardSlots[0].mSlotName)));
        ShowFreqLimit(this, text);
        return;
    }
    if (!GlobalSettings::shared()->mCardSlots.empty() &&
        GlobalSettings::shared()->mCardSlots[0].mFree <
            GlobalSettings::shared()->mPersonaMinimumFreeClusters) {
        ExitScreenByName(HxStr(kHelpScreen));
        const HxStr format(GetMetString(kMetStrFreqNoSpace));
        const HxStr text(
            Rnd::MakeString(TextOrEmpty(format),
                            TextOrEmpty(GlobalSettings::shared()->mCardSlots[0].mSlotName),
                            GlobalSettings::shared()->mPersonaMinimumFreeClusters));
        ShowFreqLimit(this, text);
        return;
    }
    PushNamedScreen(HxStr(kHelpScreen));
    PushNamedScreen(HxStr(kFreqCreateScreen));
    ActivateNamedPanel(HxStr(kFreqCreateScreen));
}

// PAL: 0x002b0418
void MetLoadFreqBaseScreen::OpenFreqMakerForCreate() {
#else
// NTSC-U/C: 0x002933b8
void MetLoadFreqBaseScreen::OnCreateButton() {
#endif
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

// NTSC-U/C: 0x00296d08, PAL: 0x002b4c70
void MetLoadFreqBaseScreen::AcquireIdentityList() {
}

// NTSC-U/C: 0x00292810, PAL: 0x002ae940
void MetLoadFreqBaseScreen::BuildButtonList() {
    mButtonList->Clear();
    mButtonList->Add(HxStr(kNameButtonObject), HxStr(kNoName));
    mButtonList->Add(HxStr(kEditButtonObject), HxStr(kNoName));
    mButtonList->Add(HxStr(kCreateButtonObject), HxStr(kNoName));

    mHelpKeys.erase(mHelpKeys.begin(), mHelpKeys.end());
    mHelpKeys.push_back(MetText(kMetStrHIdName, kNamePrompt));
    mHelpKeys.push_back(MetText(kMetStrHCidEdit, kEditPrompt));
    mHelpKeys.push_back(MetText(kMetStrHIdCreate, kCreatePrompt));

    mButtonList->SetSelected(kNameButtonIndex);
}

// NTSC-U/C: 0x00296c88, PAL: 0x002b4bf0
void MetLoadFreqBaseScreen::UpdateCycleArrows() {
    const int nShowing = mIdentityList->size() >= kMinimumCyclableEntries ? 1 : 0;

    mLeftArrow->SetShowing(nShowing);
    mRightArrow->SetShowing(nShowing);

    if (nShowing != 0) {
        mLeftArrow->SetState(kArrowShownState);
        mRightArrow->SetState(kArrowShownState);
    }
}

// NTSC-U/C: 0x00292508, PAL: 0x002ae5c8
void MetLoadFreqBaseScreen::RefreshSelection() {
    Rnd::Mat *pMat = dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr(kPreviewMaterial)));
    (*mIdentityList)[mSelectedIdentity]->AttachToBurnSlot(kPreviewBurnSlot);
    pMat->mStages[kPreviewStage].SetTex(mBurnTexture);
    UpdateNameLabel();
}

// NTSC-U/C: 0x00296c00, PAL: 0x002b4b68
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

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x002b0840
void MetLoadFreqBaseScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kFreqLimitMessage) {
        if (nChoice == kChoiceRetry) {
            StartDetect();
            return;
        }
        HxStr title = GetMetString(kMetStrTLoadChar);
        MetScreenTitleScreen::SetTitle(title);
        PushNamedScreen(HxStr(kLoadFreqScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        ActivateNamedPanel(HxStr(kLoadFreqScreen));
    } else if (name == kNoCardsMessage) {
        if (nChoice == kChoiceRetry) {
            StartDetect();
            return;
        }
        mRenderer->RemoveScreen(this);
        PushNamedScreen(HxStr(kLeftGizmoSmallScreen));
        PushNamedScreen(HxStr(kTopLogoScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kMainScreen));
        ActivateNamedPanel(HxStr(kMainScreen));
    } else {
        MetMemDetectScreen::OnMsgScreenDismissed(name, nChoice);
    }
}

// PAL: 0x002b0d98
void MetLoadFreqBaseScreen::OnNoCard() {
    std::vector<HxStr> buttons;
    buttons.push_back(GetMetString(kMetStrMsgRETRY));
    buttons.push_back(GetMetString(kMetStrMsgCANCEL));
    MetMsgScreen::Show(HxStr(kNoCardsMessage),
                       GetMetString(kMetStrMsgWARNING),
                       GetMetString(kMetStrMemCheck),
                       kTwoButtons,
                       buttons,
                       this);
}

// PAL: 0x002b11e0
void MetLoadFreqBaseScreen::StartDetect() {
    std::vector<HxStr> buttons;
    MetMsgScreen::Show(HxStr(kDetectMessage),
                       GetMetString(kMetStrMsgWARNING),
                       GetMetString(kMetStrMemDetect),
                       kNoButtons,
                       buttons,
                       this);
    mRenderer->AddScreen(this);
    MetMemDetectScreen::StartDetect();
}

// PAL: 0x002b4c78
void MetLoadFreqBaseScreen::OnDetectFinished() {
    mRenderer->RemoveScreen(this);
    if (mUsingMemcardOnEnter == kUsingMemcard) {
        MetFrontEndState::shared()->mUsingMemcard = mUsingMemcardOnEnter;
    }
    if (MetFrontEndState::shared()->mUsingMemcard != 0) {
        OnCreateButton();
    }
}
#endif
