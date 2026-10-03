#include "met/metmemcardloadscreen.h"

#include <vector>

#include "game/globalsettings.h"
#include "memcard/memcardconnectstate.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metmemcardtypescreen.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "met/metscreentitlescreen.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/button.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "mcl";
// The directory the container loads from.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "memcard_load";
// The help text key. The panel title is read under the same key.
static const char *const kCardKey = "mcl_card";

// Container objects.
static const char *const kInfoView = "memcard_info.view";
static const char *const kPanelTitleText = "mcl_memcard_pan_title.txt";
static const char *const kLeftArrow = "mcl_left_01.but";
static const char *const kRightArrow = "mcl_right_01.but";
static const char *const kSlotNumberText = "mcl_card_slot_number.txt";
static const char *const kAvailableText = "mcl_available.txt";
static const char *const kInstructionsText = "mcl_instructions.txt";

// Texts and presets.
static const char *const kPanelTitleKey = "mem_card_select";
static const char *const kNoCardKey = "mc_sel_none";
static const char *const kCardSelectedKey = "mc_sel_card";
#ifndef VIDEO_STANDARD_PAL
static const char *const kAvailableFormat = "%i kb available";
#endif
static const char *const kOnlyBackPreset = "only_back_title";
static const char *const kOptionsPreset = "mc_opt";

// Dialogue names, keys, titles, and button labels.
static const char *const kDetectMessage = "mem_load";
static const char *const kDetectKey = "mem_detect12";
static const char *const kNoCardMessage = "mem_check12";
static const char *const kOtherCardKey = "mem_detect_special";
static const char *const kWarningTitle = "WARNING";
static const char *const kOkButton = "OK";
#ifdef VIDEO_STANDARD_PAL
static const char *const kNewCardMessage = "new_card_warning";
static const char *const kMsgScreen = "MetMsgScreen";

// The two texts the European release empties.
static const char *const kLabelText1 = "mcl_label_01.txt";
static const char *const kLabelText2 = "mcl_label_02.txt";
#endif

// The slot name the card in port 1 reports when no multitap is attached.
static const char *const kPlainPortOneName = "1";

static const char *const kOwnScreenName = "MetMemCardLoadScreen";
static const char *const kConfigOptionsScreen = "MetConfigOptionsButtonsScreen";
static const char *const kMemCardTypeScreen = "MetMemCardTypeScreen";
static const char *const kLeftGizmoScreen = "MetLeftGizmoScreen";
static const char *const kRightGizmoScreen = "MetRightGizmoScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kNoName = "";

// Configuration codes the dialogue texts and the titles are read under.
constexpr int kDialogueConfigCode = 0x258;
constexpr int kTitleConfigCode = 0x269;

// Button counts MetMsgScreen receives with each dialogue.
constexpr int kNoButtons = 0;
constexpr int kOneButton = 1;
#ifdef VIDEO_STANDARD_PAL
constexpr int kTwoButtons = 2;

// The `new_card_warning` button that proceeds, counted from zero.
constexpr int kChoiceOk = 1;

// The MetFrontEndState::mUsingMemcard value OnDetectFinished() restores.
constexpr int kUsingMemcard = 1;
#endif

// Packed port and slot values of GlobalSettings::mCardSlots entries.
constexpr int kPortSlotOneA = 0;
constexpr int kPortSlotOneB = 1;
constexpr int kPortSlotTwo = 0x100;

// Cards the list needs before either arrow or cycle sound responds.
constexpr unsigned int kMinimumCyclableCards = 2;

// The command that closes the list and probes again.
constexpr int kCommandProbeAgain = 7;

// What MetScreen::mExitChoice records for slot 36 to act on.
constexpr int kExitBack = 0;
constexpr int kExitProbeAgain = 1;
constexpr int kExitToCardType = 2;

// The Rnd::Button state both arrows show while they respond.
constexpr int kButtonStateSelected = 1;

// The alternation an arrow command starts.
constexpr float kArrowAlternateInterval = 30.0f;
constexpr int kArrowAlternateCycles = 2;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// Resolve one named object of the renderer as T.
template <class T>
inline T *FindObject(const char *pszName) {
    return dynamic_cast<T *>(Rnd::TheManager.Find(HxStr(pszName)));
}

// Append the first entry of GlobalSettings::mCardSlots after index 0 with a packed port and slot.
inline void AppendCardAt(std::vector<MemcardConnectState> &cards, int nPortSlot) {
    for (unsigned int i = 1; i < GlobalSettings::shared()->mCardSlots.size(); ++i) {
        if (GlobalSettings::shared()->mCardSlots[i].mPortSlot == nPortSlot) {
            cards.push_back(GlobalSettings::shared()->mCardSlots[i]);
            return;
        }
    }
}

} // namespace

// NTSC-U/C: 0x002cb8c8, PAL: 0x002ec7c8
MetMemCardLoadScreen::MetMemCardLoadScreen(MetRenderer *pRenderer, int nPriority)
    : MetMemDetectScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mLeftArrow(nullptr), mRightArrow(nullptr), mSelected(0), mUnused(0), mPickerUser(nullptr) {
    mHelpKeys.push_back(MetText(kMetStrHMclCard, kCardKey));
}

// NTSC-U/C: 0x002cbcc8, PAL: 0x002ecc50
MetMemCardLoadScreen::~MetMemCardLoadScreen() {
}

// NTSC-U/C: 0x002d1e28, PAL: 0x002f4000
MetMemCardLoadScreen *MetMemCardLoadScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMemCardLoadScreen(pRenderer, nPriority);
}

// NTSC-U/C: 0x002cbeb8, PAL: 0x002ece58
void MetMemCardLoadScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mInfoView = FindObject<Rnd::View>(kInfoView);
    FindObject<Rnd::Text>(kPanelTitleText)
        ->SetText(MetConfigText(kMetStrMemCardSelect, kDialogueConfigCode, kPanelTitleKey));
#ifdef VIDEO_STANDARD_PAL
    // Yes, the binary does not test either text for null.
    FindObject<Rnd::Text>(kLabelText1)->SetText(HxStr(kNoName));
    FindObject<Rnd::Text>(kLabelText2)->SetText(HxStr(kNoName));
#endif
    mLeftArrow = FindObject<Rnd::Button>(kLeftArrow);
    mRightArrow = FindObject<Rnd::Button>(kRightArrow);
    mSlotNumberText = FindObject<Rnd::Text>(kSlotNumberText);
    mAvailableText = FindObject<Rnd::Text>(kAvailableText);
}

// NTSC-U/C: 0x002cc328, PAL: 0x002ed610
void MetMemCardLoadScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kMetScreenCommandLeft:
        if (mCards.size() < kMinimumCyclableCards) {
            break;
        }
        ActivateNamedPanel(HxStr(kNoName));
        StartRepeatingSound(
            mRenderer->mAnimationFrame, kArrowAlternateInterval, mLeftArrow, kArrowAlternateCycles);
        break;

    case kMetScreenCommandRight:
        if (mCards.size() < kMinimumCyclableCards) {
            break;
        }
        ActivateNamedPanel(HxStr(kNoName));
        StartRepeatingSound(mRenderer->mAnimationFrame,
                            kArrowAlternateInterval,
                            mRightArrow,
                            kArrowAlternateCycles);
        break;

    case kMetScreenCommandSelect:
        if (mCards.size() == 0) {
            break;
        }
        ActivateNamedPanel(HxStr(kNoName));
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitToCardType;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kRightGizmoScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        BeginExit();
        break;

    case kMetScreenCommandBack:
        MetHelpScreen::SetText(HxStr(kNoName), mRenderer->mAnimationFrame);
        mExitChoice = kExitBack;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kRightGizmoScreen));
        BeginExit();
        break;

    case kCommandProbeAgain:
        mExitChoice = kExitProbeAgain;
        mSelected = 0;
        ExitScreenByName(HxStr(kTitleScreen));
        ExitScreenByName(HxStr(kRightGizmoScreen));
        ExitScreenByName(HxStr(kHelpScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// NTSC-U/C: 0x002cc9c0, PAL: 0x002ede18
void MetMemCardLoadScreen::EnterAndShow() {
#ifdef VIDEO_STANDARD_PAL
    mUsingMemcardOnEnter = MetFrontEndState::shared()->mUsingMemcard;
#endif
    SetShowing(0);
    if (MetFrontEndState::shared()->mReturnScreen == kConfigOptionsScreen) {
        MetFrontEndState::shared()->mReturnScreen = HxStr(kNoName);
#ifdef VIDEO_STANDARD_PAL
        // OnMsgScreenDismissed() probes once the warning is accepted.
        std::vector<HxStr> buttons;
        buttons.push_back(GetMetString(kMetStrMsgCANCEL));
        buttons.push_back(GetMetString(kMetStrMsgOK));
        const HxStr format(GetMetString(kMetStrMemCardLoadWarning));
        const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(FirstCardSlotName())));
        MetMsgScreen::Show(HxStr(kNewCardMessage),
                           GetMetString(kMetStrMsgWARNING),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        mRenderer->AddScreen(this);
#else
        mPickerUser = this; // Yes, the binary stores this before Present() stores it again.
        Present(this, false);
#endif
    } else {
        ShowCards();
    }
}

// NTSC-U/C: 0x002ccab8, PAL: 0x002ee410
void MetMemCardLoadScreen::ShowCards() {
    MetScreenTitleScreen::SetTitle(MetConfigText(kMetStrTMclCard, kTitleConfigCode, kCardKey));
    RefreshCards();
    UpdateArrows();
    ShowSelection();
    PushNamedScreen(HxStr(kRightGizmoScreen));
    PushNamedScreen(HxStr(kHelpScreen));
    MetScreen::EnterAndShow();
}

// NTSC-U/C: 0x002ccc48, PAL: 0x002ee5f8
void MetMemCardLoadScreen::ShowSelection() {
    Rnd::Text *pInstructions = FindObject<Rnd::Text>(kInstructionsText);
    if (mCards.size() == 0) {
        mInfoView->SetShowing(0);
        pInstructions->SetText(MetConfigText(kMetStrMcSelNone, kDialogueConfigCode, kNoCardKey));
        MetHelpScreen::SelectPreset(MetText(kMetStrHOnlyBackTitle, kOnlyBackPreset));
    } else {
        mInfoView->SetShowing(1);
        mSlotNumberText->SetText(mCards[mSelected].mSlotName);
#ifdef VIDEO_STANDARD_PAL
        const HxStr format(GetMetString(kMetStrMcSpaceAvail));
        mAvailableText->SetText(
            HxStr(Rnd::MakeString(TextOrEmpty(format), mCards[mSelected].mFree)));
#else
        mAvailableText->SetText(HxStr(Rnd::MakeString(kAvailableFormat, mCards[mSelected].mFree)));
#endif
        pInstructions->SetText(
            MetConfigText(kMetStrMcSelCard, kDialogueConfigCode, kCardSelectedKey));
        MetHelpScreen::SelectPreset(MetText(kMetStrHMcOpt, kOptionsPreset));
    }
}

// NTSC-U/C: 0x002ccfb8, PAL: 0x002eea70
void MetMemCardLoadScreen::OnRepeatingSoundFinished(Rnd::Button *pButton) {
    const int nCards = mCards.size();
    if (pButton == mLeftArrow) {
        mSelected = mSelected - 1 > -1 ? mSelected - 1 : nCards - 1;
    } else if (pButton == mRightArrow) {
        mSelected = mSelected + 1 < nCards ? mSelected + 1 : 0;
    }
    ShowSelection();
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

// NTSC-U/C: 0x002cd0e8, PAL: 0x002eebc0
void MetMemCardLoadScreen::OnExitFinished() {
    if (mExitChoice == kExitBack) {
        PushNamedScreen(HxStr(kRightGizmoScreen));
        PushNamedScreen(HxStr(kConfigOptionsScreen));
        ActivateNamedPanel(HxStr(kConfigOptionsScreen));
    } else if (mExitChoice == kExitProbeAgain) {
        mPickerUser = this; // Yes, the binary stores this before Present() stores it again.
        Present(this, false);
    } else {
        static_cast<MetMemCardTypeScreen *>(FindScreenByName(HxStr(kMemCardTypeScreen)))
            ->SetCardSlot(mCards[mSelected]);
        PushNamedScreen(HxStr(kLeftGizmoScreen));
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kMemCardTypeScreen));
        ActivateNamedPanel(HxStr(kMemCardTypeScreen));
    }
}

// NTSC-U/C: 0x002cd4e0, PAL: 0x002ef098
void MetMemCardLoadScreen::OnNoCard() {
    MemcardConnectState slot;
    bool bOtherCard = false;
    if (GlobalSettings::shared()->mCardSlots.size() != 0) {
        slot = GlobalSettings::shared()->mCardSlots[0];
        if (slot.mPortSlot == kPortSlotOneB || slot.mPortSlot == kPortSlotTwo) {
            bOtherCard = true;
        }
    }

    if (!bOtherCard) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
        MetMsgScreen::ShowActive(
            HxStr(kNoCardMessage),
            MetText(kMetStrMsgWARNING, kWarningTitle),
            MetConfigText(kMetStrMemCheck12, kDialogueConfigCode, kNoCardMessage),
            kOneButton,
            buttons,
            this);
        return;
    }

    const MemcardConnectState next(NextCardSlot(slot));
    const HxStr format(MetConfigText(kMetStrMemDetectSpecial, kDialogueConfigCode, kOtherCardKey));
    const HxStr text(Rnd::MakeString(
        TextOrEmpty(format), TextOrEmpty(next.mSlotName), TextOrEmpty(slot.mSlotName)));
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
    MetMsgScreen::ShowActive(HxStr(kNoCardMessage),
                             MetText(kMetStrMsgWARNING, kWarningTitle),
                             text,
                             kOneButton,
                             buttons,
                             this);
}

// NTSC-U/C: 0x002cdce0, PAL: 0x002ef9b0
void MetMemCardLoadScreen::OnDetectFinished() {
#ifdef VIDEO_STANDARD_PAL
    if (mUsingMemcardOnEnter == kUsingMemcard) {
        MetFrontEndState::shared()->mUsingMemcard = mUsingMemcardOnEnter;
    }
#endif
    RefreshCards();
    PushNamedScreen(HxStr(kOwnScreenName));
    ActivateNamedPanel(HxStr(kOwnScreenName));
}

// NTSC-U/C: 0x002cde00, PAL: 0x002efb38
void MetMemCardLoadScreen::RefreshCards() {
    mCards.clear();
    if (GlobalSettings::shared()->mCardSlots.size() != 0) {
        if (GlobalSettings::shared()->mCardSlots[0].mPortSlot == kPortSlotOneA) {
            mCards.push_back(GlobalSettings::shared()->mCardSlots[0]);
            if (GlobalSettings::shared()->mCardSlots[0].mSlotName == kPlainPortOneName) {
                AppendCardAt(mCards, kPortSlotTwo);
            } else {
                AppendCardAt(mCards, kPortSlotOneB);
            }
        } else if (GlobalSettings::shared()->mCardSlots[0].mPortSlot == kPortSlotOneB ||
                   GlobalSettings::shared()->mCardSlots[0].mPortSlot == kPortSlotTwo) {
            mCards.push_back(GlobalSettings::shared()->mCardSlots[0]);
        }
    }
    if (static_cast<unsigned int>(mSelected) >= mCards.size()) {
        mSelected = 0;
    }
}

// NTSC-U/C: 0x002ce160, PAL: 0x002efea8
void MetMemCardLoadScreen::Present(MetMemCardPickerUser *pUser, bool bShowNow) {
    mPickerUser = pUser;
    if (bShowNow) {
        PushNamedScreen(HxStr(kOwnScreenName));
        ActivateNamedPanel(HxStr(kOwnScreenName));
    } else {
        if (mView != nullptr) {
            SetShowing(0);
        }
        StartDetect();
    }
}

// NTSC-U/C: 0x002ce2c0, PAL: 0x002f0048
void MetMemCardLoadScreen::StartDetect() {
    std::vector<HxStr> buttons;
    MetMsgScreen::Show(HxStr(kDetectMessage),
                       MetText(kMetStrMsgWARNING, kWarningTitle),
                       MetConfigText(kMetStrMemDetect12, kDialogueConfigCode, kDetectKey),
                       kNoButtons,
                       buttons,
                       this);
    mRenderer->AddScreen(this);
    MetMemDetectScreen::StartDetect();
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x002f0368
void MetMemCardLoadScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kNewCardMessage) {
        if (nChoice == kChoiceOk) {
            mPickerUser = this; // Yes, the binary stores this before Present() stores it again.
            Present(this, false);
        } else {
            PushNamedScreen(HxStr(kRightGizmoScreen));
            PushNamedScreen(HxStr(kHelpScreen));
            PushNamedScreen(HxStr(kConfigOptionsScreen));
            ActivateNamedPanel(HxStr(kConfigOptionsScreen));
        }
    } else {
        MetMemDetectScreen::OnMsgScreenDismissed(name, nChoice);
    }
}

// PAL: 0x002f4290
void MetMemCardLoadScreen::ShowFormatCheck(const MemcardConnectState &) {
    ExitScreenByName(HxStr(kMsgScreen));
}
#endif

// NTSC-U/C: 0x002d1eb0, PAL: 0x002f4088
void MetMemCardLoadScreen::PlayCycleLeftSound(int nSelector) {
    if (mCards.size() >= kMinimumCyclableCards) {
        MetScreen::PlayCycleLeftSound(nSelector);
    }
}

// NTSC-U/C: 0x002d1ef8, PAL: 0x002f40d0
void MetMemCardLoadScreen::PlayCycleRightSound(int nSelector) {
    if (mCards.size() >= kMinimumCyclableCards) {
        MetScreen::PlayCycleRightSound(nSelector);
    }
}

// NTSC-U/C: 0x002d1f40, PAL: 0x002f4118
void MetMemCardLoadScreen::PlaySlideSound(int nSelector) {
    if (!mCards.empty()) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// NTSC-U/C: 0x002d1f88, PAL: 0x002f4160
void MetMemCardLoadScreen::UpdateArrows() {
    if (mCards.size() >= kMinimumCyclableCards) {
        mLeftArrow->SetShowing(1);
        mLeftArrow->SetState(kButtonStateSelected);
        mRightArrow->SetShowing(1);
        mRightArrow->SetState(kButtonStateSelected);
    } else {
        mLeftArrow->SetShowing(0);
        mRightArrow->SetShowing(0);
    }
}

// NTSC-U/C: 0x002d2018, PAL: 0x002f41f0
void MetMemCardLoadScreen::OnEnterFinished() {
    MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
    MetHelpScreen::SelectPreset(MetText(kMetStrHMcOpt, kOptionsPreset));
}
