#include "met/metsaveremix.h"

#include <vector>

#include "game/globalsettings.h"
#include "memcard/memcardmanager.h"
#include "memcard/memcardop.h"
#include "met/metmsgscreen.h"
#include "met/metsonglists.h"
#include "met/metstrings.h"
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
#ifdef VIDEO_STANDARD_PAL
static const char *const kSaveNoFormatDialogue = "save_fail_no_format";
#endif
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

// The most remixes one card stores. The `mem_remix_2many` text also receives the count.
constexpr int kMaxRemixes = 50;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A dialogue text read by value from configuration.
inline HxStr ConfigText(MetStringId nId, const char *pszKey) {
    HxStr value = MetConfigText(nId, kDialogueConfigCode, pszKey);
    return value;
}

} // namespace

// NTSC-U/C: 0x00372120, PAL: 0x003a0c30
MetSaveRemix::MetSaveRemix(MetRenderer *pRenderer,
                           int nPriority,
                           const HxStr &name,
                           const HxStr &directory,
                           const HxStr &file)
    : MetScreen(pRenderer, nPriority, name, directory, file), mOwnerPad(0),
      mRefreshFirstCardSlot(0), mCopying(0), mAlbumNumber(0) {
}

// NTSC-U/C: 0x00372248, PAL: 0x003a0d60
MetSaveRemix::~MetSaveRemix() {
}

// NTSC-U/C: 0x00372488, PAL: 0x003a0fd0
void MetSaveRemix::RecordPendingSave(const MemcardConnectState &selection,
                                     int nOwnerPad,
                                     HxStr remixName,
                                     HxStr levelName,
                                     std::vector<FreqAppearance> appearances,
                                     int nAlbumNumber) {
    mOwnerPad = nOwnerPad;
    mTargetSlot = selection;
    mRemixName = remixName;
    mLevelName = levelName;
    mAppearances = appearances;
    mAlbumNumber = nAlbumNumber;
    mRefreshFirstCardSlot = 0;
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateGetConnectStateTask(selection.mPortSlot);
}

// NTSC-U/C: 0x0037a650, PAL: 0x003aa228
void MetSaveRemix::OnKeyboardTextEntered(const HxStr &text) {
    mRemixName = text;
    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateGetConnectStateTask(mTargetSlot.mPortSlot);
    BeginSave();
    MetMsgScreen::SetOwnerPad(mOwnerPad);
}

// NTSC-U/C: 0x0037a618, PAL: 0x003aa0c8
void MetSaveRemix::OnSaveAbandoned() {
}

// NTSC-U/C: 0x0037a620, PAL: 0x003aa0d0
void MetSaveRemix::OnSaveDialogueClosed() {
}

// NTSC-U/C: 0x0037a628, PAL: 0x003aa200
void MetSaveRemix::OnDuplicateNameDeclined() {
    OnSaveAbandoned();
}

// NTSC-U/C: 0x00372c10, PAL: 0x003a1840
void MetSaveRemix::OnConnectState(MemcardConnectState state, int nStatus) {
#ifdef VIDEO_STANDARD_PAL
    // The European release also offers to format a card the enquiry reports as unformatted.
    if (nStatus == kMemcardStatusOk || nStatus == kMemcardStatusNotFormatted) {
        if (nStatus == kMemcardStatusOk && state.mFormatted != 0) {
#else
    if (nStatus == kMemcardStatusOk) {
        if (state.mFormatted != 0) {
#endif
            if (mRefreshFirstCardSlot != 0) {
                GlobalSettings::shared(); // Yes, the binary discards this call's result.
                GlobalSettings::shared()->mCardSlots[0] = state;
                mRefreshFirstCardSlot = 0;
                ExitScreenByName(HxStr(kMsgScreen));
            } else {
                mCardRemixes.clear();
                MemcardManager::shared()->CreateListRemixesTask(mTargetSlot.mPortSlot,
                                                                &mCardRemixes);
                BeginSave();
            }
            return;
        }

        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
        buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
        HxStr format;
        HxStr text;
        if (mCopying != 0) {
            format = ConfigText(kMetStrCopyFailFormat, kCopyFailFormatText);
        } else {
            format = ConfigText(kMetStrSaveFailFormat, kSaveFailFormatText);
        }
        text = FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName));
        MetMsgScreen::Show(HxStr(kFormatCheckDialogue),
                           MetText(kMetStrMsgWARNING, kWarningTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        MetMsgScreen::SetOwnerPad(mOwnerPad);
        return;
    }

    std::vector<HxStr> buttons;
    HxStr format;
    HxStr text;
    if (mCopying != 0) {
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCANCEL, kCancelButton));
        format = ConfigText(kMetStrCopyFailNocard, kCopyFailNoCardText);
    } else {
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        format = ConfigText(kMetStrSaveFailNocard, kSaveFailNoCardText);
    }
    text = FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName));
    MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                       MetText(kMetStrMsgERROR, kErrorTitle),
                       text,
                       kTwoButtons,
                       buttons,
                       this);
    MetMsgScreen::SetOwnerPad(mOwnerPad);
}

// NTSC-U/C: 0x00373808, PAL: 0x003a2b28
void MetSaveRemix::OnCardFormatted([[maybe_unused]] int nPortSlot, int nStatus) {
    switch (nStatus) {
    case kMemcardStatusOk: {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        const HxStr format(ConfigText(kMetStrFormatSuccess, kFormatSuccessText));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName)));
        MetMsgScreen::ShowActive(HxStr(kFormatDoneDialogue),
                                 MetText(kMetStrMsgWARNING, kWarningTitle),
                                 text,
                                 kOneButton,
                                 buttons,
                                 this);
        MetMsgScreen::SetOwnerPad(mOwnerPad);
        break;
    }

    case kCardStatusAlreadyFormatted: {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        const HxStr format(ConfigText(kMetStrFormatAlready, kFormatAlreadyText));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName)));
        MetMsgScreen::ShowActive(HxStr(kFormatAlreadyDialogue),
                                 MetText(kMetStrMsgWARNING, kWarningTitle),
                                 text,
                                 kOneButton,
                                 buttons,
                                 this);
        MetMsgScreen::SetOwnerPad(mOwnerPad);
        break;
    }

    default: {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgBACK, kBackButton));
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           ConfigText(kMetStrFormatFail, kFormatFailText),
                           kTwoButtons,
                           buttons,
                           this);
        MetMsgScreen::SetOwnerPad(mOwnerPad);
        break;
    }
    }
}

// NTSC-U/C: 0x00374b58, PAL: 0x003a4158
#ifdef VIDEO_STANDARD_PAL
void MetSaveRemix::OnRemixSaved([[maybe_unused]] int nPortSlot, int nStatus, int nKilobytes) {
#else
void MetSaveRemix::OnRemixSaved([[maybe_unused]] int nPortSlot, int nStatus) {
#endif
    switch (nStatus) {
    case kMemcardStatusOk:
#ifdef VIDEO_STANDARD_PAL
        if (mTargetSlot.mPortSlot == kFirstCardPortSlot &&
            GlobalSettings::shared()->mCardSlots.size() != 0) {
#else
        if (mTargetSlot.mPortSlot == kFirstCardPortSlot) {
#endif
            mRefreshFirstCardSlot = 1;
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateGetConnectStateTask(mTargetSlot.mPortSlot);
        } else {
            ExitScreenByName(HxStr(kMsgScreen));
        }
        break;

    case kMemcardStatusCardFull: {
        std::vector<HxStr> buttons;
        if (mCopying != 0) {
            buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
            buttons.push_back(MetText(kMetStrMsgCANCEL, kCancelButton));
            const HxStr format(ConfigText(kMetStrCopyFailNospace, kCopyNoSpaceText));
            const HxStr text(FormatString(TextOrEmpty(format),
                                          TextOrEmpty(mTargetSlot.mSlotName),
                                          GlobalSettings::shared()->mMinimumFreeClusters));
            MetMsgScreen::ShowActive(HxStr(kCopyNoSpaceDialogue),
                                     MetText(kMetStrMsgERROR, kErrorTitle),
                                     text,
                                     kTwoButtons,
                                     buttons,
                                     this);
            // Yes, the binary does not set the owner pad for the copy dialogue.
        } else {
            buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
            buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
            const HxStr format(ConfigText(kMetStrSaveFailNospace, kSaveNoSpaceText));
#ifdef VIDEO_STANDARD_PAL
            const HxStr text(
                FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName), nKilobytes));
#else
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName)));
#endif
            MetMsgScreen::ShowActive(HxStr(kSaveNoSpaceDialogue),
                                     MetText(kMetStrMsgERROR, kErrorTitle),
                                     text,
                                     kTwoButtons,
                                     buttons,
                                     this);
            MetMsgScreen::SetOwnerPad(mOwnerPad);
        }
        break;
    }

    default: {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        // Yes, the binary identifies this dialogue as `save_fail_no_space` although its text is the
        // general failure.
        MetMsgScreen::ShowActive(HxStr(kSaveNoSpaceDialogue),
                                 MetText(kMetStrMsgERROR, kErrorTitle),
                                 ConfigText(kMetStrSaveFailGeneral, kSaveFailGeneralText),
                                 kOneButton,
                                 buttons,
                                 this);
        MetMsgScreen::SetOwnerPad(mOwnerPad);
        break;
    }
    }
}

// NTSC-U/C: 0x00374208, PAL: 0x003a36b8
void MetSaveRemix::OnRemixesListed([[maybe_unused]] int nPortSlot, [[maybe_unused]] int nStatus) {
    const int nCount = mCardRemixes.size();
    bool bDuplicate = false;
    for (int i = 0; i < nCount; ++i) {
        if (mCardRemixes[i].name == mRemixName) {
            bDuplicate = true;
            break;
        }
    }

    if (bDuplicate) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
        buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
        const HxStr question(FormatString(kQuestionFormat, TextOrEmpty(mRemixName)));
        const HxStr text(ConfigText(kMetStrMemRemixDupe, kRemixDupeDialogue) + question);
        MetMsgScreen::ShowActive(HxStr(kRemixDupeDialogue),
                                 MetText(kMetStrMsgWARNING, kWarningTitle),
                                 text,
                                 kTwoButtons,
                                 buttons,
                                 this);
        MetMsgScreen::SetOwnerPad(mOwnerPad);
    } else if (nCount >= kMaxRemixes) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        const HxStr format(ConfigText(kMetStrMemRemix2many, kTooManyRemixesDialogue));
        const HxStr text(
            FormatString(TextOrEmpty(format), kMaxRemixes, TextOrEmpty(mTargetSlot.mSlotName)));
        MetMsgScreen::ShowActive(HxStr(kTooManyRemixesDialogue),
                                 MetText(kMetStrMsgERROR, kErrorTitle),
                                 text,
                                 kTwoButtons,
                                 buttons,
                                 this);
        MetMsgScreen::SetOwnerPad(mOwnerPad);
    } else {
        MemcardManager::shared()->mUser = this;
        MemcardManager::shared()->CreateSaveRemixTask(
            mTargetSlot.mPortSlot, mRemixName, mAppearances, mLevelName, mAlbumNumber);
    }
}

// NTSC-U/C: 0x00375590, PAL: 0x003a4dd8
void MetSaveRemix::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kMemCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            OnSaveAbandoned();
            return;
        }
    } else if (name == kFormatCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->CreateFormatTask(mTargetSlot.mPortSlot);
            const std::vector<HxStr> buttons;
            const HxStr format(ConfigText(kMetStrMemFormatGo, kFormatGoDialogue));
            const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName)));
            MetMsgScreen::Show(HxStr(kFormatGoDialogue),
                               MetText(kMetStrMsgWARNING, kWarningTitle),
                               text,
                               kNoButtons,
                               buttons,
                               this);
            MetMsgScreen::SetOwnerPad(mOwnerPad);
        } else {
#ifdef VIDEO_STANDARD_PAL
            std::vector<HxStr> buttons;
            buttons.push_back(GetMetString(kMetStrMsgRETRY));
            buttons.push_back(GetMetString(kMetStrMsgCONTINUE));
            MetMsgScreen::Show(HxStr(kSaveNoFormatDialogue),
                               GetMetString(kMetStrMsgERROR),
                               GetMetString(kMetStrSaveAborted),
                               kTwoButtons,
                               buttons,
                               this);
#else
            RecordPendingSave(
                mTargetSlot, mOwnerPad, mRemixName, mLevelName, mAppearances, mAlbumNumber);
#endif
        }
        return;
    } else if (name == kFormatDoneDialogue) {
        MemcardManager::shared()->mUser = this;
        MemcardManager::shared()->CreateSaveRemixTask(
            mTargetSlot.mPortSlot, mRemixName, mAppearances, mLevelName, mAlbumNumber);
        BeginSave();
        return;
    } else if (name == kFormatAlreadyDialogue) {
        // Falls through to the connect-state enquiry below.
    } else if (name == kRemixDupeDialogue) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateSaveRemixTask(
                mTargetSlot.mPortSlot, mRemixName, mAppearances, mLevelName, mAlbumNumber);
            BeginSave();
            MetMsgScreen::SetOwnerPad(mOwnerPad); // Yes, BeginSave() has already done this.
        } else {
            OnDuplicateNameDeclined();
        }
        return;
    } else if (name == kTooManyRemixesDialogue) {
        if (nChoice != kChoiceFirst) {
            OnSaveAbandoned();
            return;
        }
#ifdef VIDEO_STANDARD_PAL
    } else if (name == kSaveNoSpaceDialogue || name == kCopyNoSpaceDialogue ||
               name == kSaveNoFormatDialogue) {
#else
    } else if (name == kSaveNoSpaceDialogue || name == kCopyNoSpaceDialogue) {
#endif
        if (nChoice != kChoiceFirst) {
            OnSaveAbandoned();
            return;
        }
    } else {
        OnSaveDialogueClosed();
        return;
    }

    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateGetConnectStateTask(mTargetSlot.mPortSlot);
}

// NTSC-U/C: 0x00372760, PAL: 0x003a12e8
void MetSaveRemix::BeginSave() {
    const std::vector<HxStr> buttons;
    HxStr text;
    HxStr title;
    if (mCopying != 0) {
        title = ConfigText(kMetStrCopyTitle, kCopyTitleKey);
        const HxStr format(ConfigText(kMetStrMemCopy12, kCopyText));
        text = FormatString(TextOrEmpty(format),
                            TextOrEmpty(NextCardSlot(mTargetSlot).mSlotName),
                            TextOrEmpty(mTargetSlot.mSlotName));
    } else {
        title = ConfigText(kMetStrSaveTitle, kSaveTitleKey);
        const HxStr format(ConfigText(kMetStrMemSave, kSaveText));
        text = FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName));
    }
    MetMsgScreen::Show(HxStr(kSaveRemixDialogue), title, text, kNoButtons, buttons, this);
    MetMsgScreen::SetOwnerPad(mOwnerPad);
}
