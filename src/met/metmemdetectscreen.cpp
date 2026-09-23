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
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// The screen name, directory, and container name New() passes.
static const char *const kNoName = "";

// Message screen names, which OnMsgScreenDismissed() matches. Most double as the text key.
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

// A dialogue text read by value from configuration.
inline HxStr ConfigText(const char *pszKey) {
    HxStr value = QueryConfigString(kDialogueConfigCode, pszKey);
    return value;
}

// The name of the card GlobalSettings records.
inline const char *RecordedCardName() {
    return TextOrEmpty(GlobalSettings::shared()->mCardSlots[0].mSlotName);
}

} // namespace

// 0x002deab8
MetMemDetectScreen::MetMemDetectScreen(MetRenderer *pRenderer,
                                       int nPriority,
                                       const HxStr &name,
                                       const HxStr &directory,
                                       const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mUnknown90(0), mUnknown94(0),
      mUnknown98(0), mAutosaveNoticeTime(0) {
}

// 0x002deb08
MetMemDetectScreen::~MetMemDetectScreen() {
}

// 0x002d89c8
MetMemDetectScreen *MetMemDetectScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetMemDetectScreen(
        pRenderer, nPriority, HxStr(kNoName), HxStr(kNoName), HxStr(kNoName));
}

// 0x002d8b80
void MetMemDetectScreen::StartDetect() {
    std::vector<HxStr> buttons;
    mAutosaveNoticeTime = 0;
    MetMsgScreen::Show(HxStr(kDetectMessage),
                       HxStr(kWarningTitle),
                       ConfigText(kDetectKey),
                       kNoButtons,
                       buttons,
                       this);
    MemcardManager::shared()->mUser = this;
    GlobalSettings::shared()->mCardSlots.clear();
    MemcardManager::shared()->CreateGetAllConnectStatesTask(&GlobalSettings::shared()->mCardSlots);
}

// 0x002d8e98
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

    MetFrontEndState::shared()->mUnknown0c = 1;
    if (slot.mFormatted) {
        std::vector<HxStr> buttons;
        MetMsgScreen::Show(HxStr(kDetectMessage),
                           HxStr(kWarningTitle),
                           ConfigText(kDetectKey),
                           kNoButtons,
                           buttons,
                           this);
        MemcardManager::shared()->CreateLoadGlobalSettingsTask(slot.mPortSlot,
                                                               GlobalSettings::shared());
    } else {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        const HxStr format(ConfigText(kFormatCheckMessage));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slot.mSlotName)));
        MetMsgScreen::ShowActive(
            HxStr(kFormatCheckMessage), HxStr(kWarningTitle), text, kTwoButtons, buttons, this);
    }
}

// 0x002d9628
void MetMemDetectScreen::OnCardFormatted(int, int nStatus) {
    std::vector<HxStr> buttons;
    switch (nStatus) {
    case kCardStatusFormatted: {
        MetFrontEndState::shared()->mUnknown0c = 1;
        buttons.push_back(HxStr(kContinueButton));
        const HxStr format(ConfigText(kFormatSuccessMessage));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
        MetMsgScreen::Show(
            HxStr(kFormatSuccessMessage), HxStr(kFormatTitle), text, kOneButton, buttons, this);
        break;
    }

    case kCardStatusAlreadyFormatted: {
        MetFrontEndState::shared()->mUnknown0c = 1;
        buttons.push_back(HxStr(kContinueButton));
        const HxStr format(ConfigText(kFormatAlreadyMessage));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
        MetMsgScreen::Show(
            HxStr(kFormatAlreadyMessage), HxStr(kFormatTitle), text, kOneButton, buttons, this);
        break;
    }

    default:
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kBackButton));
        MetMsgScreen::Show(HxStr(kFormatFailMessage),
                           HxStr(kErrorTitle),
                           ConfigText(kFormatFailMessage),
                           kTwoButtons,
                           buttons,
                           this);
        break;
    }
}

// 0x002d9e40
void MetMemDetectScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kNoCardMessage) {
        if (nChoice == kChoiceSecond) {
            MetFrontEndState::shared()->mUnknown0c = 0;
            OnDetectFinished();
        } else {
            StartDetect();
        }
    } else if (name == kNoCardOkMessage) {
        MetFrontEndState::shared()->mUnknown0c = 0;
        OnDetectFinished();
    } else if (name == kFormatCheckMessage) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->CreateFormatTask(kFirstCardPortSlot);
            std::vector<HxStr> buttons;
            const HxStr format(ConfigText(kFormatGoMessage));
            GlobalSettings::shared(); // Yes, the binary discards this call's result.
            const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
            MetMsgScreen::Show(
                HxStr(kFormatGoMessage), HxStr(kWarningTitle), text, kNoButtons, buttons, this);
        } else {
            MetFrontEndState::shared()->mUnknown0c = 0;
            std::vector<HxStr> buttons;
            buttons.push_back(HxStr(kNoButton));
            buttons.push_back(HxStr(kYesButton));
            MetMsgScreen::ShowActive(HxStr(kErrorMessage),
                                     HxStr(kWarningTitle),
                                     ConfigText(kErrorMessage),
                                     kTwoButtons,
                                     buttons,
                                     this);
        }
    } else if (name == kFormatSuccessMessage) {
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        GlobalSettings::shared()->mCardSlots[0].mFree = kFormattedCardFreeClusters;
        std::vector<HxStr> buttons;
        MetMsgScreen::ShowActive(HxStr(kAutosaveMessage),
                                 HxStr(kWarningTitle),
                                 ConfigText(kAutosaveMessage),
                                 kNoButtons,
                                 buttons,
                                 this);
        mAutosaveNoticeTime = mUnknown10->mUnknown68;
    } else if (name == kFormatAlreadyMessage) {
        StartDetect();
    } else if (name == kFormatFailMessage) {
        MetFrontEndState::shared()->mUnknown0c = 0;
        StartDetect();
    } else if (name == kNoSpaceMessage) {
        if (nChoice != kChoiceFirst) {
            MetFrontEndState::shared()->mUnknown0c = 0;
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

// 0x002da868
void MetMemDetectScreen::OnMinimumSaveSpace(int, int nSpace) {
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    std::vector<HxStr> buttons;
    if (GlobalSettings::shared()->mCardSlots[0].mFree < nSpace) {
        if (nSpace == GlobalSettings::shared()->mUnknown6c) {
            buttons.push_back(HxStr(kRetryButton));
            buttons.push_back(HxStr(kContinueButton));
            const HxStr format(ConfigText(kNoSpaceKey));
            const HxStr cardName(FirstCardSlotName());
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(cardName)));
            MetMsgScreen::ShowActive(
                HxStr(kNoSpaceMessage), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
        } else {
            buttons.push_back(HxStr(kRetryButton));
            buttons.push_back(HxStr(kContinueButton));
            const HxStr format(ConfigText(kNeedsSpaceKey));
            const HxStr cardName(FirstCardSlotName());
            const HxStr text(FormatString(TextOrEmpty(format),
                                          TextOrEmpty(cardName),
                                          nSpace - GlobalSettings::shared()->mCardSlots[0].mFree));
            MetMsgScreen::ShowActive(
                HxStr(kNeedsSpaceMessage), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
        }
    } else {
        MetMsgScreen::ShowActive(HxStr(kAutosaveMessage),
                                 HxStr(kWarningTitle),
                                 ConfigText(kAutosaveMessage),
                                 kNoButtons,
                                 buttons,
                                 this);
        mAutosaveNoticeTime = mUnknown10->mUnknown68;
    }
}

// 0x002db328
void MetMemDetectScreen::StartLoadPersonas() {
    mUnknown98 = 1;
    std::vector<HxStr> buttons;
    MetPersonaData::ClearLoadList();
    const HxStr format(ConfigText(kLoadKey));
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
    MetMsgScreen::Show(HxStr(kLoadMessage), HxStr(kLoadingTitle), text, kNoButtons, buttons, this);
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateLoadPersonasTask(
        GlobalSettings::shared()->mCardSlots[0].mPortSlot, MetPersonaData::loadList());
}

// 0x002deb70
void MetMemDetectScreen::OnGlobalSettingsLoaded(int, int) {
    StartLoadPersonas();
}

// 0x002deb98
void MetMemDetectScreen::OnPersonasLoaded(int, int) {
    StartSaveSpaceCheck();
}

// 0x002debc0
void MetMemDetectScreen::StartSaveSpaceCheck() {
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateMinimumSaveSpaceTask(
        GlobalSettings::shared()->mCardSlots[0].mPortSlot);
}

// 0x002dec10
void MetMemDetectScreen::OnUnknownSlot26(float flTime) {
    if (mAutosaveNoticeTime != 0 && mAutosaveNoticeTime + kAutosaveNoticeDuration < flTime) {
        mAutosaveNoticeTime = 0;
        ExitScreenByName(HxStr(kMsgScreen));
    }
}
