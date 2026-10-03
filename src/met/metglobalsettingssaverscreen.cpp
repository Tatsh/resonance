#include "met/metglobalsettingssaverscreen.h"

#include "game/globalsettings.h"
#include "memcard/formatcardmct.h"
#include "memcard/memcardmanager.h"
#include "memcard/memcardop.h"
#include "met/metfrontendstate.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "met/metstrings.h"
#include "os/formatstring.h"
#include "os/hxstr.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "dlg";
// The directory the container loads from. The capital S is what the image records.
static const char *const kDirectory = "metagame/Shared";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "dialogue";

// The registry keys StartSave() resolves.
static const char *const kOwnScreenName = "MetGlobalSettingsSaverScreen";
static const char *const kSonyScreen = "MetSonyScreen";
static const char *const kMsgScreen = "MetMsgScreen";

// Configuration code the dialogue texts are read under.
constexpr int kDialogueConfigCode = 0x258;

// Dialogue names. Each except `format_fail` is also the key of its text, a format taking the
// card's name.
static const char *const kSaveDialogue = "mem_save";
static const char *const kFormatCheckDialogue = "mem_format_check";
static const char *const kMemCheckDialogue = "mem_check";
static const char *const kFormatDoneDialogue = "mem_format_done";
static const char *const kFormatFailDialogue = "format_fail";
static const char *const kFormatGoDialogue = "mem_format_go";
#ifdef VIDEO_STANDARD_PAL
static const char *const kSaveAbortedDialogue = "save_fail_no_format";
// The slot name the European release records when the first card slot is not port 1.
static const char *const kFirstCardSlotName = "1";
#endif

// Dialogue titles.
static const char *const kSettingsTitle = "SETTINGS";
static const char *const kFormatTitle = "FORMAT";
static const char *const kErrorTitle = "ERROR";
static const char *const kWarningTitle = "WARNING";

// Configuration keys of the texts that differ from their dialogue names.
static const char *const kFormatSuccessText = "format_success";
static const char *const kFormatAlreadyText = "format_already";
static const char *const kSaveFailNoCardText = "save_fail_nocard";
static const char *const kSaveFailNoSpaceText = "save_fail_nospace";
static const char *const kSaveFailText = "save_fail";

// Button labels.
static const char *const kNoButton = "NO";
static const char *const kYesButton = "YES";
static const char *const kRetryButton = "RETRY";
static const char *const kContinueButton = "CONTINUE";

// Button counts MetMsgScreen receives with each dialogue.
constexpr int kNoButtons = 0;
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;

// The dialogue buttons OnMsgScreenDismissed() tests, counted from zero.
constexpr int kChoiceFirst = 0;
constexpr int kChoiceSecond = 1;

// The packed port and slot of port 1, the card the format task receives.
constexpr int kFirstCardPortSlot = 0;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// The name of the card GlobalSettings records.
inline const char *RecordedCardName() {
    return TextOrEmpty(GlobalSettings::shared()->mCardSlots[0].mSlotName);
}

} // namespace

// 0x0027c4f8
MetGlobalSettingsSaverScreen::MetGlobalSettingsSaverScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)) {
}

// 0x0027c680
MetGlobalSettingsSaverScreen::~MetGlobalSettingsSaverScreen() {
}

// 0x00281fc0
MetGlobalSettingsSaverScreen *MetGlobalSettingsSaverScreen::New(MetRenderer *pRenderer,
                                                                int nPriority) {
    return new MetGlobalSettingsSaverScreen(pRenderer, nPriority);
}

// 0x0027c2e0
void MetGlobalSettingsSaverScreen::StartSave(const std::vector<HxStr> &screens) {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    MetGlobalSettingsSaverScreen *pSaver =
        pScreen != nullptr ? dynamic_cast<MetGlobalSettingsSaverScreen *>(pScreen) : nullptr;
    // The binary does not test the result for null.
    pSaver->SetReturnScreens(screens);

    MetScreen *pSony = MetScreen::FindScreenByName(HxStr(kSonyScreen));
    if (MetFrontEndState::shared()->mUsingMemcard != 0) {
        pSony->PushNamedScreen(HxStr(kOwnScreenName));
        return;
    }
    int nCount = screens.size();
    for (int i = 0; i < nCount; ++i) {
        pSony->PushNamedScreen(screens[i]);
    }
    pSony->ActivateNamedPanel(screens[0]);
}

// 0x00282048
void MetGlobalSettingsSaverScreen::SetReturnScreens(const std::vector<HxStr> &screens) {
    mReturnScreens = screens;
}

// 0x00282088
void MetGlobalSettingsSaverScreen::EnterAndShow() {
    RequestConnectState();
}

// NTSC-U/C: 0x002820a8, PAL: 0x00295540
inline void MetGlobalSettingsSaverScreen::RequestConnectState() {
    MemcardManager::shared()->mUser = this;
#ifdef VIDEO_STANDARD_PAL
    // The European release enquires about the card in port 1 whatever card GlobalSettings records.
    MemcardConnectState state;
    if (GlobalSettings::shared()->mCardSlots.size() != 0 &&
        GlobalSettings::shared()->mCardSlots[0].mPortSlot == kFirstCardPortSlot) {
        state = GlobalSettings::shared()->mCardSlots[0];
    } else {
        state.mPortSlot = kFirstCardPortSlot;
        state.mSlotName = kFirstCardSlotName;
    }
    MemcardManager::shared()->CreateGetConnectStateTask(state.mPortSlot);
#else
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MemcardManager::shared()->CreateGetConnectStateTask(
        GlobalSettings::shared()->mCardSlots[0].mPortSlot);
#endif
}

// 0x002820f8
void MetGlobalSettingsSaverScreen::BeginExit() {
    mRenderer->RemoveScreen(this);
    int nCount = mReturnScreens.size();
    for (int i = 0; i < nCount; ++i) {
        PushNamedScreen(mReturnScreens[i]);
    }
    ActivateNamedPanel(mReturnScreens[0]);
}

// 0x00282068
void MetGlobalSettingsSaverScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
}

// The binary expands the routine in place in OnConnectState().
inline void MetGlobalSettingsSaverScreen::ShowFormatCheck(const MemcardConnectState &state) {
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
    buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
    const HxStr format(
        MetConfigText(kMetStrMemFormatCheck, kDialogueConfigCode, kFormatCheckDialogue));
    const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(state.mSlotName)));
    MetMsgScreen::Show(HxStr(kFormatCheckDialogue),
                       MetText(kMetStrMsgSETTINGS, kSettingsTitle),
                       text,
                       kTwoButtons,
                       buttons,
                       this);
}

// NTSC-U/C: 0x0027c7a8, PAL: 0x002956b8
void MetGlobalSettingsSaverScreen::OnConnectState(MemcardConnectState state, int nStatus) {
    if (nStatus == kMemcardStatusOk) {
        if (state.mFormatted != 0) {
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateSaveGlobalSettingsTask(state.mPortSlot,
                                                                   GlobalSettings::shared());
            const std::vector<HxStr> buttons;
            const HxStr format(MetConfigText(kMetStrMemSave, kDialogueConfigCode, kSaveDialogue));
            const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
            MetMsgScreen::Show(HxStr(kSaveDialogue),
                               MetText(kMetStrMsgSETTINGS, kSettingsTitle),
                               text,
                               kNoButtons,
                               buttons,
                               this);
            return;
        }

        ShowFormatCheck(state);
        return;
    }

#ifdef VIDEO_STANDARD_PAL
    if (nStatus == kMemcardStatusNotFormatted) {
        ShowFormatCheck(state);
        return;
    }
#endif

    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
    buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
    MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                       MetText(kMetStrMsgSETTINGS, kSettingsTitle),
                       MetConfigText(kMetStrMemCheck, kDialogueConfigCode, kMemCheckDialogue),
                       kTwoButtons,
                       buttons,
                       this);
}

// NTSC-U/C: 0x0027d258, PAL: 0x00296800
void MetGlobalSettingsSaverScreen::OnCardFormatted([[maybe_unused]] int nPortSlot, int nStatus) {
    switch (nStatus) {
    case kMemcardStatusOk: {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr format(
            MetConfigText(kMetStrFormatSuccess, kDialogueConfigCode, kFormatSuccessText));
        const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
        MetMsgScreen::ShowActive(HxStr(kFormatDoneDialogue),
                                 MetText(kMetStrMsgFORMAT, kFormatTitle),
                                 text,
                                 kOneButton,
                                 buttons,
                                 this);
        break;
    }

    case kMemcardStatusAlreadyFormatted: {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr format(
            MetConfigText(kMetStrFormatAlready, kDialogueConfigCode, kFormatAlreadyText));
        const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
        MetMsgScreen::ShowActive(HxStr(kFormatDoneDialogue),
                                 MetText(kMetStrMsgFORMAT, kFormatTitle),
                                 text,
                                 kOneButton,
                                 buttons,
                                 this);
        break;
    }

    default: {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        MetMsgScreen::Show(
            HxStr(kFormatFailDialogue),
            MetText(kMetStrMsgERROR, kErrorTitle),
            MetConfigText(kMetStrFormatFail, kDialogueConfigCode, kFormatFailDialogue),
            kTwoButtons,
            buttons,
            this);
        break;
    }
    }
}

// NTSC-U/C: 0x0027dc70, PAL: 0x002973a8
void MetGlobalSettingsSaverScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kMemCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            BeginExit();
            return;
        }
    } else if (name == kFormatCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            MemcardManager::shared()->CreateFormatTask(kFirstCardPortSlot);
            const std::vector<HxStr> buttons;
            GlobalSettings::shared(); // Yes, the binary discards this call's result.
            const HxStr format(
                MetConfigText(kMetStrMemFormatGo, kDialogueConfigCode, kFormatGoDialogue));
            const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
            MetMsgScreen::Show(HxStr(kFormatGoDialogue),
                               MetText(kMetStrMsgWARNING, kWarningTitle),
                               text,
                               kNoButtons,
                               buttons,
                               this);
            return;
        }
#ifdef VIDEO_STANDARD_PAL
        std::vector<HxStr> buttons;
        buttons.push_back(GetMetString(kMetStrMsgRETRY));
        buttons.push_back(GetMetString(kMetStrMsgCONTINUE));
        MetMsgScreen::Show(HxStr(kSaveAbortedDialogue),
                           GetMetString(kMetStrMsgERROR),
                           GetMetString(kMetStrSaveAborted),
                           kTwoButtons,
                           buttons,
                           this);
        return;
#endif
    } else if (name == kFormatDoneDialogue) {
        // Falls through to the connect-state enquiry below.
    } else if (name == kFormatFailDialogue) {
        if (nChoice != kChoiceFirst) {
            BeginExit();
            return;
        }
#ifdef VIDEO_STANDARD_PAL
    } else if (name == kSaveAbortedDialogue) {
        if (nChoice != kChoiceFirst) {
            BeginExit();
            return;
        }
#endif
    } else {
        BeginExit();
        return;
    }

    RequestConnectState();
}

// NTSC-U/C: 0x0027e080, PAL: 0x00297bc8
#ifdef VIDEO_STANDARD_PAL
void MetGlobalSettingsSaverScreen::OnGlobalSettingsSaved([[maybe_unused]] int nPortSlot,
                                                         int nStatus,
                                                         int nKilobytes) {
#else
void MetGlobalSettingsSaverScreen::OnGlobalSettingsSaved([[maybe_unused]] int nPortSlot,
                                                         int nStatus) {
#endif
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
    buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
    HxStr format;
    HxStr text;
    switch (nStatus) {
    case kMemcardStatusOk:
        ExitScreenByName(HxStr(kMsgScreen));
        break;

    case kMemcardStatusUnknown:
        format = MetConfigText(kMetStrSaveFailNocard, kDialogueConfigCode, kSaveFailNoCardText);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        text = FormatString(TextOrEmpty(format), RecordedCardName());
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           MetText(kMetStrMsgSETTINGS, kSettingsTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        break;

    case kMemcardStatusCardFull:
        format = MetConfigText(kMetStrSaveFailNospace, kDialogueConfigCode, kSaveFailNoSpaceText);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
#ifdef VIDEO_STANDARD_PAL
        text = FormatString(TextOrEmpty(format), RecordedCardName(), nKilobytes);
#else
        text = FormatString(TextOrEmpty(format), RecordedCardName());
#endif
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           MetText(kMetStrMsgSETTINGS, kSettingsTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        break;

    default:
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           MetText(kMetStrMsgSETTINGS, kSettingsTitle),
                           MetConfigText(kMetStrSaveFail, kDialogueConfigCode, kSaveFailText),
                           kTwoButtons,
                           buttons,
                           this);
        break;
    }
}
