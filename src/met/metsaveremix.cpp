#include "met/metsaveremix.h"

#include <vector>

#include "game/globalsettings.h"
#include "memcard/memcardmanager.h"
#include "memcard/memcardop.h"
#include "met/metmsgscreen.h"
#include "met/metsonglists.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/configquery.h"

namespace {

// Configuration code the dialogue texts are read under.
constexpr int kDialogueConfigCode = 0x258;

// Dialogue names.
static const char *const kSaveRemixDialogue = "save_remix";
static const char *const kMemCheckDialogue = "mem_check";
static const char *const kFormatCheckDialogue = "mem_format_check";
static const char *const kFormatDoneDialogue = "mem_format_done";
static const char *const kFormatAlreadyDialogue = "mem_format_already";
static const char *const kFormatGoDialogue = "mem_format_go";
static const char *const kRemixDupeDialogue = "mem_remix_dupe";
static const char *const kTooManyRemixesDialogue = "mem_remix_2many";
static const char *const kSaveNoSpaceDialogue = "save_fail_no_space";
static const char *const kCopyNoSpaceDialogue = "copy_fail_no_space";
static const char *const kMsgScreen = "MetMsgScreen";

// Dialogue titles, and the configuration keys of the two BeginSave() reads.
static const char *const kWarningTitle = "WARNING";
static const char *const kErrorTitle = "ERROR";
static const char *const kSaveTitleKey = "save_title";
static const char *const kCopyTitleKey = "copy_title";

// Configuration keys of the dialogue texts, each a format taking the target card's name.
static const char *const kSaveText = "mem_save";
static const char *const kCopyText = "mem_copy12";
static const char *const kSaveFailFormatText = "save_fail_format";
static const char *const kCopyFailFormatText = "copy_fail_format";
static const char *const kSaveFailNoCardText = "save_fail_nocard";
static const char *const kCopyFailNoCardText = "copy_fail_nocard";
static const char *const kFormatSuccessText = "format_success";
static const char *const kFormatAlreadyText = "format_already";
static const char *const kFormatFailText = "format_fail";
static const char *const kSaveNoSpaceText = "save_fail_nospace";
static const char *const kCopyNoSpaceText = "copy_fail_nospace";
static const char *const kSaveFailGeneralText = "save_fail_general";

// The question appended to the `mem_remix_dupe` text, filled with the remix name.
static const char *const kQuestionFormat = "%s?";

// Button labels.
static const char *const kNoButton = "NO";
static const char *const kYesButton = "YES";
static const char *const kRetryButton = "RETRY";
static const char *const kCancelButton = "CANCEL";
static const char *const kContinueButton = "CONTINUE";
static const char *const kBackButton = "BACK";

// Button counts MetMsgScreen receives with each dialogue.
constexpr int kNoButtons = 0;
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;

// The format status FormatCardMCT reports for a card that was already formatted. The name is
// inferred from the dialogue it raises.
constexpr int kCardStatusAlreadyFormatted = 13;

// The dialogue buttons OnMsgScreenDismissed() tests, counted from zero.
constexpr int kChoiceFirst = 0;
constexpr int kChoiceSecond = 1;

// The packed port and slot of port 1, the card GlobalSettings::mCardSlots records.
constexpr int kFirstCardPortSlot = 0;

// The most remixes one card holds, which the `mem_remix_2many` text also receives.
constexpr int kMaxRemixes = 50;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A dialogue text read by value from configuration.
inline HxStr ConfigText(const char *pszKey) {
    HxStr value;
    QueryConfigString(&value, kDialogueConfigCode, pszKey);
    return value;
}

} // namespace

// 0x00372120
MetSaveRemix::MetSaveRemix(MetRenderer *pRenderer,
                           int nPriority,
                           const HxStr &name,
                           const HxStr &directory,
                           const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mUnknownc8(0), mUnknownd8(0),
      mUnknowndc(0), mUnknowne4(0) {
}

// 0x00372248
MetSaveRemix::~MetSaveRemix() {
}

// 0x0037a618
void MetSaveRemix::OnUnknownSlot40() {
}

// 0x0037a620
void MetSaveRemix::OnUnknownSlot41() {
}

// 0x0037a628
void MetSaveRemix::OnUnknownSlot42() {
    OnUnknownSlot40();
}

// 0x00372c10
void MetSaveRemix::OnConnectState(MemcardConnectState state, int nStatus) {
    if (nStatus == kMemcardStatusOk) {
        if (state.mFormatted != 0) {
            if (mUnknownd8 != 0) {
                GlobalSettings::shared(); // Yes, the binary discards this call's result.
                GlobalSettings::shared()->mCardSlots[0] = state;
                mUnknownd8 = 0;
                ExitScreenByName(HxStr(kMsgScreen));
            } else {
                mUnknowncc.clear();
                MemcardManager::shared()->CreateListRemixesTask(mUnknown94.mPortSlot, &mUnknowncc);
                BeginSave();
            }
            return;
        }

        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        HxStr format;
        HxStr text;
        if (mUnknowndc != 0) {
            format = ConfigText(kCopyFailFormatText);
        } else {
            format = ConfigText(kSaveFailFormatText);
        }
        text = FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown94.mSlotName));
        MetMsgScreen::Show(
            HxStr(kFormatCheckDialogue), HxStr(kWarningTitle), text, kTwoButtons, buttons, this);
        MetMsgScreen::SetOwnerPad(mUnknownc8);
        return;
    }

    std::vector<HxStr> buttons;
    HxStr format;
    HxStr text;
    if (mUnknowndc != 0) {
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kCancelButton));
        format = ConfigText(kCopyFailNoCardText);
    } else {
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        format = ConfigText(kSaveFailNoCardText);
    }
    text = FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown94.mSlotName));
    MetMsgScreen::Show(
        HxStr(kMemCheckDialogue), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
    MetMsgScreen::SetOwnerPad(mUnknownc8);
}

// 0x00373808
void MetSaveRemix::OnCardFormatted([[maybe_unused]] int nPortSlot, int nStatus) {
    switch (nStatus) {
    case kMemcardStatusOk: {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kContinueButton));
        const HxStr format(ConfigText(kFormatSuccessText));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown94.mSlotName)));
        MetMsgScreen::ShowActive(
            HxStr(kFormatDoneDialogue), HxStr(kWarningTitle), text, kOneButton, buttons, this);
        MetMsgScreen::SetOwnerPad(mUnknownc8);
        break;
    }

    case kCardStatusAlreadyFormatted: {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kContinueButton));
        const HxStr format(ConfigText(kFormatAlreadyText));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown94.mSlotName)));
        MetMsgScreen::ShowActive(
            HxStr(kFormatAlreadyDialogue), HxStr(kWarningTitle), text, kOneButton, buttons, this);
        MetMsgScreen::SetOwnerPad(mUnknownc8);
        break;
    }

    default: {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kBackButton));
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           HxStr(kErrorTitle),
                           ConfigText(kFormatFailText),
                           kTwoButtons,
                           buttons,
                           this);
        MetMsgScreen::SetOwnerPad(mUnknownc8);
        break;
    }
    }
}

// 0x00374b58
void MetSaveRemix::OnRemixSaved([[maybe_unused]] int nPortSlot, int nStatus) {
    switch (nStatus) {
    case kMemcardStatusOk:
        if (mUnknown94.mPortSlot == kFirstCardPortSlot) {
            mUnknownd8 = 1;
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateGetConnectStateTask(mUnknown94.mPortSlot);
        } else {
            ExitScreenByName(HxStr(kMsgScreen));
        }
        break;

    case kMemcardStatusCardFull: {
        std::vector<HxStr> buttons;
        if (mUnknowndc != 0) {
            buttons.push_back(HxStr(kRetryButton));
            buttons.push_back(HxStr(kCancelButton));
            const HxStr format(ConfigText(kCopyNoSpaceText));
            const HxStr text(FormatString(TextOrEmpty(format),
                                          TextOrEmpty(mUnknown94.mSlotName),
                                          GlobalSettings::shared()->mMinimumFreeClusters));
            MetMsgScreen::ShowActive(
                HxStr(kCopyNoSpaceDialogue), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
            MetMsgScreen::SetOwnerPad(mUnknownc8);
        } else {
            buttons.push_back(HxStr(kRetryButton));
            buttons.push_back(HxStr(kContinueButton));
            const HxStr format(ConfigText(kSaveNoSpaceText));
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown94.mSlotName)));
            MetMsgScreen::ShowActive(
                HxStr(kSaveNoSpaceDialogue), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
            MetMsgScreen::SetOwnerPad(mUnknownc8);
        }
        break;
    }

    default: {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kContinueButton));
        // Yes, the binary names this dialogue `save_fail_no_space` although its text is the
        // general failure.
        MetMsgScreen::ShowActive(HxStr(kSaveNoSpaceDialogue),
                                 HxStr(kErrorTitle),
                                 ConfigText(kSaveFailGeneralText),
                                 kOneButton,
                                 buttons,
                                 this);
        MetMsgScreen::SetOwnerPad(mUnknownc8);
        break;
    }
    }
}

// 0x00374208
void MetSaveRemix::OnRemixesListed([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
    const int nCount = mUnknowncc.size();
    bool bDuplicate = false;
    for (int i = 0; i < nCount; ++i) {
        if (mUnknowncc[i].name == mUnknownb8) {
            bDuplicate = true;
            break;
        }
    }

    if (bDuplicate) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        const HxStr question(FormatString(kQuestionFormat, TextOrEmpty(mUnknownb8)));
        const HxStr text(ConfigText(kRemixDupeDialogue) + question);
        MetMsgScreen::ShowActive(
            HxStr(kRemixDupeDialogue), HxStr(kWarningTitle), text, kTwoButtons, buttons, this);
        MetMsgScreen::SetOwnerPad(mUnknownc8);
    } else if (nCount >= kMaxRemixes) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        const HxStr format(ConfigText(kTooManyRemixesDialogue));
        const HxStr text(
            FormatString(TextOrEmpty(format), kMaxRemixes, TextOrEmpty(mUnknown94.mSlotName)));
        MetMsgScreen::ShowActive(
            HxStr(kTooManyRemixesDialogue), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
        MetMsgScreen::SetOwnerPad(mUnknownc8);
    } else {
        MemcardManager::shared()->mUser = this;
        MemcardManager::shared()->CreateSaveRemixTask(
            mUnknown94.mPortSlot, mUnknownb8, mUnknownac, mUnknownc0, mUnknowne4);
    }
}

// 0x00375590
void MetSaveRemix::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kMemCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            OnUnknownSlot40();
            return;
        }
    } else if (name == kFormatCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->CreateFormatTask(mUnknown94.mPortSlot);
            const std::vector<HxStr> buttons;
            const HxStr format(ConfigText(kFormatGoDialogue));
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown94.mSlotName)));
            MetMsgScreen::Show(
                HxStr(kFormatGoDialogue), HxStr(kWarningTitle), text, kNoButtons, buttons, this);
            MetMsgScreen::SetOwnerPad(mUnknownc8);
        } else {
            OnUnknownSlot39(mUnknown94, mUnknownc8, mUnknownb8, mUnknownc0, mUnknownac, mUnknowne4);
        }
        return;
    } else if (name == kFormatDoneDialogue) {
        MemcardManager::shared()->mUser = this;
        MemcardManager::shared()->CreateSaveRemixTask(
            mUnknown94.mPortSlot, mUnknownb8, mUnknownac, mUnknownc0, mUnknowne4);
        BeginSave();
        return;
    } else if (name == kFormatAlreadyDialogue) {
        // Falls through to the connect-state enquiry below.
    } else if (name == kRemixDupeDialogue) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateSaveRemixTask(
                mUnknown94.mPortSlot, mUnknownb8, mUnknownac, mUnknownc0, mUnknowne4);
            BeginSave();
            MetMsgScreen::SetOwnerPad(mUnknownc8); // Yes, BeginSave() has already done this.
        } else {
            OnUnknownSlot42();
        }
        return;
    } else if (name == kTooManyRemixesDialogue) {
        if (nChoice != kChoiceFirst) {
            OnUnknownSlot40();
            return;
        }
    } else if (name == kSaveNoSpaceDialogue || name == kCopyNoSpaceDialogue) {
        if (nChoice != kChoiceFirst) {
            OnUnknownSlot40();
            return;
        }
    } else {
        OnUnknownSlot41();
        return;
    }

    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateGetConnectStateTask(mUnknown94.mPortSlot);
}

// 0x00372760
void MetSaveRemix::BeginSave() {
    const std::vector<HxStr> buttons;
    HxStr text;
    HxStr title;
    if (mUnknowndc != 0) {
        title = ConfigText(kCopyTitleKey);
        const HxStr format(ConfigText(kCopyText));
        text = FormatString(TextOrEmpty(format),
                            TextOrEmpty(NextCardSlot(mUnknown94).mSlotName),
                            TextOrEmpty(mUnknown94.mSlotName));
    } else {
        title = ConfigText(kSaveTitleKey);
        const HxStr format(ConfigText(kSaveText));
        text = FormatString(TextOrEmpty(format), TextOrEmpty(mUnknown94.mSlotName));
    }
    MetMsgScreen::Show(HxStr(kSaveRemixDialogue), title, text, kNoButtons, buttons, this);
    MetMsgScreen::SetOwnerPad(mUnknownc8);
}
