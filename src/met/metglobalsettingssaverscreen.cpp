#include "met/metglobalsettingssaverscreen.h"

#include "game/globalsettings.h"
#include "memcard/formatcardmct.h"
#include "memcard/memcardmanager.h"
#include "memcard/memcardop.h"
#include "met/metfrontendstate.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "script/configquery.h"

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

// A dialogue text read by value from configuration.
inline HxStr ConfigText(const char *pszKey) {
    HxStr value;
    QueryConfigString(&value, kDialogueConfigCode, pszKey);
    return value;
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
    if (MetFrontEndState::shared()->mUnknown0c != 0) {
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

// 0x002820a8
inline void MetGlobalSettingsSaverScreen::RequestConnectState() {
    MemcardManager::shared()->mUser = this;
    GlobalSettings::shared(); // Yes, the binary discards this call's result.
    MemcardManager::shared()->CreateGetConnectStateTask(
        GlobalSettings::shared()->mCardSlots[0].mPortSlot);
}

// 0x002820f8
void MetGlobalSettingsSaverScreen::BeginExit() {
    mUnknown10->RemoveScreen(this);
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

// 0x0027c7a8
void MetGlobalSettingsSaverScreen::OnConnectState(MemcardConnectState state, int nStatus) {
    if (nStatus == kMemcardStatusOk) {
        if (state.mFormatted != 0) {
            MemcardManager::shared()->mUser = this;
            MemcardManager::shared()->CreateSaveGlobalSettingsTask(state.mPortSlot,
                                                                   GlobalSettings::shared());
            const std::vector<HxStr> buttons;
            const HxStr format(ConfigText(kSaveDialogue));
            const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
            MetMsgScreen::Show(
                HxStr(kSaveDialogue), HxStr(kSettingsTitle), text, kNoButtons, buttons, this);
            return;
        }

        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        const HxStr format(ConfigText(kFormatCheckDialogue));
        const HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(state.mSlotName)));
        MetMsgScreen::Show(
            HxStr(kFormatCheckDialogue), HxStr(kSettingsTitle), text, kTwoButtons, buttons, this);
        return;
    }

    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kRetryButton));
    buttons.push_back(HxStr(kContinueButton));
    MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                       HxStr(kSettingsTitle),
                       ConfigText(kMemCheckDialogue),
                       kTwoButtons,
                       buttons,
                       this);
}

// 0x0027d258
void MetGlobalSettingsSaverScreen::OnCardFormatted([[maybe_unused]] int nPortSlot, int nStatus) {
    switch (nStatus) {
    case kMemcardStatusOk: {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kContinueButton));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr format(ConfigText(kFormatSuccessText));
        const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
        MetMsgScreen::ShowActive(
            HxStr(kFormatDoneDialogue), HxStr(kFormatTitle), text, kOneButton, buttons, this);
        break;
    }

    case kMemcardStatusAlreadyFormatted: {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kContinueButton));
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        const HxStr format(ConfigText(kFormatAlreadyText));
        const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
        MetMsgScreen::ShowActive(
            HxStr(kFormatDoneDialogue), HxStr(kFormatTitle), text, kOneButton, buttons, this);
        break;
    }

    default: {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        MetMsgScreen::Show(HxStr(kFormatFailDialogue),
                           HxStr(kErrorTitle),
                           ConfigText(kFormatFailDialogue),
                           kTwoButtons,
                           buttons,
                           this);
        break;
    }
    }
}

// 0x0027dc70
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
            const HxStr format(ConfigText(kFormatGoDialogue));
            const HxStr text(FormatString(TextOrEmpty(format), RecordedCardName()));
            MetMsgScreen::Show(
                HxStr(kFormatGoDialogue), HxStr(kWarningTitle), text, kNoButtons, buttons, this);
            return;
        }
    } else if (name == kFormatDoneDialogue) {
        // Falls through to the connect-state enquiry below.
    } else if (name == kFormatFailDialogue) {
        if (nChoice != kChoiceFirst) {
            BeginExit();
            return;
        }
    } else {
        BeginExit();
        return;
    }

    RequestConnectState();
}

// 0x0027e080
void MetGlobalSettingsSaverScreen::OnGlobalSettingsSaved([[maybe_unused]] int nPortSlot,
                                                         int nStatus) {
    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kRetryButton));
    buttons.push_back(HxStr(kContinueButton));
    HxStr format;
    HxStr text;
    switch (nStatus) {
    case kMemcardStatusOk:
        ExitScreenByName(HxStr(kMsgScreen));
        break;

    case kMemcardStatusUnknown:
        format = ConfigText(kSaveFailNoCardText);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        text = FormatString(TextOrEmpty(format), RecordedCardName());
        MetMsgScreen::Show(
            HxStr(kMemCheckDialogue), HxStr(kSettingsTitle), text, kTwoButtons, buttons, this);
        break;

    case kMemcardStatusCardFull:
        format = ConfigText(kSaveFailNoSpaceText);
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        text = FormatString(TextOrEmpty(format), RecordedCardName());
        MetMsgScreen::Show(
            HxStr(kMemCheckDialogue), HxStr(kSettingsTitle), text, kTwoButtons, buttons, this);
        break;

    default:
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           HxStr(kSettingsTitle),
                           ConfigText(kSaveFailText),
                           kTwoButtons,
                           buttons,
                           this);
        break;
    }
}
