#include "met/metmemdetectscreen.h"

#include <vector>

#include "game/globalsettings.h"
#include "memcard/memcardconnectstate.h"
#include "memcard/memcardmanager.h"
#include "met/metfrontendstate.h"
#include "met/metmsgscreen.h"
#include "met/metpersonadata.h"
#include "met/metrenderer.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"

namespace {

// The screen name, directory, and container name New() passes.
static const char *const kNoName = "";

// Message screen names that OnMsgScreenDismissed() matches. Most double as the text key.
static const char *const kDetectMessage = "mem_load";
static const char *const kNoCardMessage = "mem_check";
static const char *const kNoCardOkMessage = "mem_check12";
static const char *const kFormatCheckMessage = "mem_format_check";
static const char *const kFormatGoMessage = "mem_format_go";
static const char *const kFormatSuccessMessage = "format_success";
static const char *const kFormatAlreadyMessage = "format_already";
static const char *const kFormatFailMessage = "format_fail";
static const char *const kNoSpaceMessage = "warn_game_no_space";
static const char *const kNeedsSpaceMessage = "warn_game_needs_space";
static const char *const kErrorMessage = "mem_error";
static const char *const kAutosaveMessage = "mem_autosave";
static const char *const kLoadMessage = "load";

// Text keys that differ from the message screen name.
static const char *const kDetectKey = "mem_detect";
static const char *const kLoadKey = "mem_load";
static const char *const kNoSpaceKey = "mem_nospace";
static const char *const kNeedsSpaceKey = "mem_needs_space";

// Dialogue titles.
static const char *const kWarningTitle = "WARNING";
static const char *const kErrorTitle = "ERROR";
static const char *const kFormatTitle = "FORMAT";
static const char *const kLoadingTitle = "LOADING";

// Button labels.
static const char *const kNoButton = "NO";
static const char *const kYesButton = "YES";
static const char *const kRetryButton = "RETRY";
static const char *const kContinueButton = "CONTINUE";
static const char *const kBackButton = "BACK";

static const char *const kMsgScreen = "MetMsgScreen";

// The configuration code every dialogue text is read under.
constexpr int kDialogueConfigCode = 0x258;

// Button counts MetMsgScreen receives with each dialogue.
constexpr int kNoButtons = 0;
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;

// The dialogue buttons OnMsgScreenDismissed() tests, counted from zero.
constexpr int kChoiceFirst = 0;
constexpr int kChoiceSecond = 1;

// The packed port and slot of port 1, the card GlobalSettings::mCardSlots records.
constexpr int kFirstCardPortSlot = 0;

// The format results OnCardFormatted() tells apart. The names are inferred from the dialogues.
constexpr int kCardStatusFormatted = 0;
constexpr int kCardStatusAlreadyFormatted = 13;

// The free clusters recorded for a card that has just been formatted.
constexpr int kFormattedCardFreeClusters = 8000;

// How long the autosave notice stays up, in units of renderer time.
constexpr float kAutosaveNoticeDuration = 480.0f;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// The name of the card GlobalSettings records.
inline const char *RecordedCardName() {
    return TextOrEmpty(GlobalSettings::shared()->mCardSlots[0].mSlotName);
}

#ifdef VIDEO_STANDARD_PAL
// The nCampaign values OnMinimumSaveSpace() tests, and the free clusters each requires.
constexpr int kFullSaveCheck = 0;
constexpr int kCampaignSaveCheck = 1;
constexpr int kFullSaveClusters = 128;
constexpr int kCampaignSaveClusters = 50;

// The no-space warning the European OnMinimumSaveSpace() expands for both requirements.
inline void ShowNoSpaceWarning(MetScreen *pOwner, MetStringId nFormatId, int nClusters) {
    std::vector<HxStr> buttons;
    buttons.push_back(GetMetString(kMetStrMsgRETRY));
    buttons.push_back(GetMetString(kMetStrMsgCONTINUE));
    const HxStr format(GetMetString(nFormatId));
    const HxStr cardName(FirstCardSlotName());
    const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(cardName), nClusters));
    MetMsgScreen::ShowActive(
        HxStr(kNoSpaceMessage), GetMetString(kMetStrMsgERROR), text, kTwoButtons, buttons, pOwner);
}
#endif

} // namespace

// NTSC-U/C: 0x002deab8, PAL: 0x003017e0
MetMemDetectScreen::MetMemDetectScreen(MetRenderer *pRenderer,
                                       int nPriority,
                                       const HxStr &name,
                                       const HxStr &directory,
                                       const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mUnused1(0), mUnused2(0),
      mPersonaLoadRequested(0), mAutosaveNoticeTime(0) {
}

// NTSC-U/C: 0x002deb08, PAL: 0x00301830
MetMemDetectScreen::~MetMemDetectScreen() {
}

// NTSC-U/C: 0x002d89c8, PAL: 0x002fb180
MetMemDetectScreen *MetMemDetectScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMemDetectScreen(
        pRenderer, nPriority, HxStr(kNoName), HxStr(kNoName), HxStr(kNoName));
}

// NTSC-U/C: 0x002d8b80, PAL: 0x002fb390
void MetMemDetectScreen::StartDetect() {
    mAutosaveNoticeTime = 0;
#ifndef VIDEO_STANDARD_PAL
    std::vector<HxStr> buttons;
    MetMsgScreen::Show(HxStr(kDetectMessage),
                       HxStr(kWarningTitle),
                       MetConfigText(kMetStrMemDetect, kDialogueConfigCode, kDetectKey),
                       kNoButtons,
                       buttons,
                       this);
#endif
    MemcardManager::shared()->mUser = this;
    GlobalSettings::shared()->mCardSlots.clear();
    MemcardManager::shared()->CreateGetAllConnectStatesTask(&GlobalSettings::shared()->mCardSlots);
}

// NTSC-U/C: 0x002d8e98, PAL: 0x002fb458
void MetMemDetectScreen::OnAllConnectStates() {
    if (GlobalSettings::shared()->mCardSlots.size() == 0) {
        OnNoCard();
        return;
    }

    const MemcardConnectState slot = GlobalSettings::shared()->mCardSlots[0];
    if (slot.mPortSlot != kFirstCardPortSlot) {
        OnNoCard();
        return;
    }

    MetFrontEndState::shared()->mUsingMemcard = 1;
    if (slot.mFormatted) {
        std::vector<HxStr> buttons;
        MetMsgScreen::Show(HxStr(kDetectMessage),
                           MetText(kMetStrMsgWARNING, kWarningTitle),
                           MetConfigText(kMetStrMemDetect, kDialogueConfigCode, kDetectKey),
                           kNoButtons,
                           buttons,
                           this);
        MemcardManager::shared()->CreateLoadGlobalSettingsTask(slot.mPortSlot,
                                                               GlobalSettings::shared());
    } else {
#ifdef VIDEO_STANDARD_PAL
        ShowFormatCheck(slot);
#else
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        const HxStr format(
            MetConfigText(kMetStrMemFormatCheck, kDialogueConfigCode, kFormatCheckMessage));
        const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(slot.mSlotName)));
        MetMsgScreen::ShowActive(
            HxStr(kFormatCheckMessage), HxStr(kWarningTitle), text, kTwoButtons, buttons, this);
#endif
    }
}

#ifdef VIDEO_STANDARD_PAL
// PAL: 0x002fde10
void MetMemDetectScreen::ShowFormatCheck(const MemcardConnectState &slot) {
    std::vector<HxStr> buttons;
    buttons.push_back(GetMetString(kMetStrMsgNO));
    buttons.push_back(GetMetString(kMetStrMsgYES));
    const HxStr format(GetMetString(kMetStrMemFormatCheck));
    const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(slot.mSlotName)));
    MetMsgScreen::ShowActive(HxStr(kFormatCheckMessage),
                             GetMetString(kMetStrMsgWARNING),
                             text,
                             kTwoButtons,
                             buttons,
                             this);
}
#endif

// NTSC-U/C: 0x002d9628, PAL: 0x002fb8c8
void MetMemDetectScreen::OnCardFormatted(int, int nStatus) {
    std::vector<HxStr> buttons;
    switch (nStatus) {
    case kCardStatusFormatted: {
        MetFrontEndState::shared()->mUsingMemcard = 1;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        const HxStr format(
            MetConfigText(kMetStrFormatSuccess, kDialogueConfigCode, kFormatSuccessMessage));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr text(Rnd::MakeString(TextOrEmpty(format), RecordedCardName()));
        MetMsgScreen::Show(HxStr(kFormatSuccessMessage),
                           MetText(kMetStrMsgFORMAT, kFormatTitle),
                           text,
                           kOneButton,
                           buttons,
                           this);
        break;
    }

    case kCardStatusAlreadyFormatted: {
        MetFrontEndState::shared()->mUsingMemcard = 1;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        const HxStr format(
            MetConfigText(kMetStrFormatAlready, kDialogueConfigCode, kFormatAlreadyMessage));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr text(Rnd::MakeString(TextOrEmpty(format), RecordedCardName()));
        MetMsgScreen::Show(HxStr(kFormatAlreadyMessage),
                           MetText(kMetStrMsgFORMAT, kFormatTitle),
                           text,
                           kOneButton,
                           buttons,
                           this);
        break;
    }

    default:
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgBACK, kBackButton));
        MetMsgScreen::Show(
            HxStr(kFormatFailMessage),
            MetText(kMetStrMsgERROR, kErrorTitle),
            MetConfigText(kMetStrFormatFail, kDialogueConfigCode, kFormatFailMessage),
            kTwoButtons,
            buttons,
            this);
        break;
    }
}

// NTSC-U/C: 0x002d9e40, PAL: 0x002fc220
void MetMemDetectScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kNoCardMessage) {
        if (nChoice == kChoiceSecond) {
            MetFrontEndState::shared()->mUsingMemcard = 0;
            OnDetectFinished();
        } else {
            StartDetect();
        }
    } else if (name == kNoCardOkMessage) {
        MetFrontEndState::shared()->mUsingMemcard = 0;
        OnDetectFinished();
    } else if (name == kFormatCheckMessage) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->CreateFormatTask(kFirstCardPortSlot);
            std::vector<HxStr> buttons;
            const HxStr format(
                MetConfigText(kMetStrMemFormatGo, kDialogueConfigCode, kFormatGoMessage));
            GlobalSettings::shared(); // Yes, the binary discards this call's result.
            const HxStr text(Rnd::MakeString(TextOrEmpty(format), RecordedCardName()));
            MetMsgScreen::Show(HxStr(kFormatGoMessage),
                               MetText(kMetStrMsgWARNING, kWarningTitle),
                               text,
                               kNoButtons,
                               buttons,
                               this);
        } else {
#ifndef VIDEO_STANDARD_PAL
            MetFrontEndState::shared()->mUsingMemcard = 0;
#endif
            std::vector<HxStr> buttons;
            buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
            buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
            MetMsgScreen::ShowActive(
                HxStr(kErrorMessage),
                MetText(kMetStrMsgWARNING, kWarningTitle),
                MetConfigText(kMetStrMemError, kDialogueConfigCode, kErrorMessage),
                kTwoButtons,
                buttons,
                this);
        }
    } else if (name == kFormatSuccessMessage) {
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        GlobalSettings::shared()->mCardSlots[0].mFree = kFormattedCardFreeClusters;
        std::vector<HxStr> buttons;
        MetMsgScreen::ShowActive(
            HxStr(kAutosaveMessage),
            MetText(kMetStrMsgWARNING, kWarningTitle),
            MetConfigText(kMetStrMemAutosave, kDialogueConfigCode, kAutosaveMessage),
            kNoButtons,
            buttons,
            this);
        mAutosaveNoticeTime = mRenderer->mAnimationFrame;
    } else if (name == kFormatAlreadyMessage) {
        StartDetect();
    } else if (name == kFormatFailMessage) {
        MetFrontEndState::shared()->mUsingMemcard = 0;
        StartDetect();
    } else if (name == kNoSpaceMessage) {
        if (nChoice != kChoiceFirst) {
            MetFrontEndState::shared()->mUsingMemcard = 0;
            OnDetectFinished();
        } else {
            StartDetect();
        }
    } else if (name == kNeedsSpaceMessage) {
        if (nChoice != kChoiceFirst) {
            OnDetectFinished();
        } else {
            StartDetect();
        }
    } else if (name == kErrorMessage && nChoice == kChoiceFirst) {
        StartDetect();
    } else {
        OnDetectFinished();
    }
}

#ifdef VIDEO_STANDARD_PAL
// NTSC-U/C: 0x002da868, PAL: 0x002fcd88
void MetMemDetectScreen::OnMinimumSaveSpace(int, int, int nSkipWarning, int nCampaign) {
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    std::vector<HxStr> buttons;
    if (nSkipWarning == 0 && nCampaign == kFullSaveCheck &&
        GlobalSettings::shared()->mCardSlots[0].mFree < kFullSaveClusters) {
        ShowNoSpaceWarning(this, kMetStrMemNospace, kFullSaveClusters);
    } else if (nSkipWarning == 0 && nCampaign == kCampaignSaveCheck &&
               GlobalSettings::shared()->mCardSlots[0].mFree < kCampaignSaveClusters) {
        ShowNoSpaceWarning(this, kMetStrMemNospaceForCampaign, kCampaignSaveClusters);
    } else {
        MetMsgScreen::ShowActive(HxStr(kAutosaveMessage),
                                 GetMetString(kMetStrMsgWARNING),
                                 GetMetString(kMetStrMemAutosave),
                                 kNoButtons,
                                 buttons,
                                 this);
        mAutosaveNoticeTime = mRenderer->mAnimationFrame;
    }
}
#else
// NTSC-U/C: 0x002da868
void MetMemDetectScreen::OnMinimumSaveSpace(int, int nSpace) {
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    std::vector<HxStr> buttons;
    if (GlobalSettings::shared()->mCardSlots[0].mFree < nSpace) {
        if (nSpace == GlobalSettings::shared()->mRequiredSaveSpace) {
            buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
            buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
            const HxStr format(MetConfigText(kMetStrMemNospace, kDialogueConfigCode, kNoSpaceKey));
            const HxStr cardName(FirstCardSlotName());
            const HxStr text(Rnd::MakeString(TextOrEmpty(format), TextOrEmpty(cardName)));
            MetMsgScreen::ShowActive(HxStr(kNoSpaceMessage),
                                     MetText(kMetStrMsgERROR, kErrorTitle),
                                     text,
                                     kTwoButtons,
                                     buttons,
                                     this);
        } else {
            buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
            buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
            const HxStr format(
                MetConfigText(kMetStrMemNospaceForCampaign, kDialogueConfigCode, kNeedsSpaceKey));
            const HxStr cardName(FirstCardSlotName());
            const HxStr text(
                Rnd::MakeString(TextOrEmpty(format),
                                TextOrEmpty(cardName),
                                nSpace - GlobalSettings::shared()->mCardSlots[0].mFree));
            MetMsgScreen::ShowActive(HxStr(kNeedsSpaceMessage),
                                     MetText(kMetStrMsgERROR, kErrorTitle),
                                     text,
                                     kTwoButtons,
                                     buttons,
                                     this);
        }
    } else {
        MetMsgScreen::ShowActive(
            HxStr(kAutosaveMessage),
            MetText(kMetStrMsgWARNING, kWarningTitle),
            MetConfigText(kMetStrMemAutosave, kDialogueConfigCode, kAutosaveMessage),
            kNoButtons,
            buttons,
            this);
        mAutosaveNoticeTime = mRenderer->mAnimationFrame;
    }
}
#endif

// NTSC-U/C: 0x002db328, PAL: 0x002fda28
void MetMemDetectScreen::StartLoadPersonas() {
    mPersonaLoadRequested = 1;
    std::vector<HxStr> buttons;
    MetPersonaData::ClearLoadList();
    const HxStr format(MetConfigText(kMetStrMemLoad, kDialogueConfigCode, kLoadKey));
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    const HxStr text(Rnd::MakeString(TextOrEmpty(format), RecordedCardName()));
    MetMsgScreen::Show(HxStr(kLoadMessage),
                       MetText(kMetStrMsgLOADING, kLoadingTitle),
                       text,
                       kNoButtons,
                       buttons,
                       this);
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateLoadPersonasTask(
        GlobalSettings::shared()->mCardSlots[0].mPortSlot, MetPersonaData::loadList());
}

// NTSC-U/C: 0x002deb70, PAL: 0x00301898
void MetMemDetectScreen::OnGlobalSettingsLoaded(int, int) {
    StartLoadPersonas();
}

// NTSC-U/C: 0x002deb98, PAL: 0x003018c0
void MetMemDetectScreen::OnPersonasLoaded(int, int) {
    StartSaveSpaceCheck();
}

// NTSC-U/C: 0x002debc0, PAL: 0x003018e8
void MetMemDetectScreen::StartSaveSpaceCheck() {
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateMinimumSaveSpaceTask(
        GlobalSettings::shared()->mCardSlots[0].mPortSlot);
}

// NTSC-U/C: 0x002dec10, PAL: 0x00301938
void MetMemDetectScreen::UpdateIdle(float flTime) {
    if (mAutosaveNoticeTime != 0 && mAutosaveNoticeTime + kAutosaveNoticeDuration < flTime) {
        mAutosaveNoticeTime = 0;
        ExitScreenByName(HxStr(kMsgScreen));
    }
}
