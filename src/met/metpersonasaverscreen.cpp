#include "met/metpersonasaverscreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/globalsettings.h"
#include "memcard/formatcardmct.h"
#include "memcard/memcardmanager.h"
#include "memcard/memcardop.h"
#include "met/metfreqmakerassetmanager.h"
#include "met/metfrontendstate.h"
#include "met/metkeyboardrequest.h"
#include "met/metkeyboardscreen.h"
#include "met/metmsgscreen.h"
#include "met/metrenderer.h"
#include "met/metsonglists.h"
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
static const char *const kOwnScreenName = "MetPersonaSaverScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
static const char *const kMsgScreen = "MetMsgScreen";

// What StartDelete() records in mUnknown94.
constexpr int kDeleteRequest = 1;

// Configuration code the dialogue texts are read under.
constexpr int kDialogueConfigCode = 0x258;

// Dialogue names.
static const char *const kMemCheckDialogue = "mem_check";
static const char *const kFormatCheckDialogue = "mem_format_check";
static const char *const kFormatDoneDialogue = "mem_format_done";
static const char *const kFormatFailDialogue = "format_fail";
static const char *const kFormatGoDialogue = "mem_format_go";
static const char *const kNoSaveWarnDialogue = "no_save_warn";
static const char *const kSaveNoSpaceDialogue = "save_fail_no_space";
static const char *const kCopyNoSpaceDialogue = "copy_fail_no_space";
static const char *const kSaveDialogue = "mem_save";
static const char *const kDeletingDialogue = "doingDelete";
static const char *const kLimitDialogue = "freq_limit";
static const char *const kNameRequiredDialogue = "new_name_required";
static const char *const kReplaceDialogue = "freq_replace";

// Dialogue titles and the configuration keys of the two progress titles.
static const char *const kWarningTitle = "WARNING";
static const char *const kErrorTitle = "ERROR";
static const char *const kSaveTitleKey = "save_title";
static const char *const kCopyTitleKey = "copy_title";

// Configuration keys of the dialogue texts.
static const char *const kSaveText = "mem_save";
static const char *const kCopyText = "mem_copy12";
static const char *const kDeleteFirstText = "freq_mid_del1";
static const char *const kDeleteSecondText = "freq_mid_del2";
static const char *const kDeleteFormat = "%s %s %s";
static const char *const kSaveNoSpaceText = "save_fail_nospace";
static const char *const kCopyNoSpaceText = "copy_fail_nospace";
static const char *const kSaveFailFormatText = "save_fail_format";
static const char *const kCopyFailFormatText = "copy_fail_format";
static const char *const kDeleteFailNoCardText = "del_fail_nocard";
static const char *const kSaveFailNoCardText = "save_fail_nocard";
static const char *const kCopyFailNoCardText = "copy_fail_nocard";
static const char *const kSaveFailGeneralText = "save_fail_general";
static const char *const kFormatSuccessText = "format_success";
static const char *const kFormatAlreadyText = "format_already";
static const char *const kFormatFailText = "format_fail";
static const char *const kFormatGoText = "mem_format_go";
static const char *const kNoSaveWarnText = "no_save_warn";
static const char *const kLimitText = "freq_limit";
static const char *const kNoNameText = "freq_no_name";
static const char *const kNewNameText = "freq_new_name";
static const char *const kDeleteNotFoundText = "del_fail_notfound";
static const char *const kReplaceText = "freq_replace";

// Button labels.
static const char *const kNoButton = "NO";
static const char *const kYesButton = "YES";
static const char *const kOkButton = "OK";
static const char *const kRetryButton = "RETRY";
static const char *const kCancelButton = "CANCEL";
static const char *const kContinueButton = "CONTINUE";
static const char *const kBackButton = "BACK";

// Button counts MetMsgScreen receives with each dialogue.
constexpr int kNoButtons = 0;
constexpr int kOneButton = 1;
constexpr int kTwoButtons = 2;

// The dialogue buttons OnMsgScreenDismissed() tests, counted from zero.
constexpr int kChoiceFirst = 0;
constexpr int kChoiceSecond = 1;

// The most personas one card holds, which the `freq_limit` text also receives.
constexpr int kMaxPersonas = 8;

// The FreQ name keyboard.
static const char *const kNamePrompt = "FreQ name";
static const char *const kNoText = "";
constexpr int kAnyPad = -1;
constexpr int kNameMaxLength = 12;
constexpr int kNameMaxWidth = 176;

// The value mUnknown94, mUnknown98, mUnknown9c, and mUnknownbc take while set.
constexpr int kFlagSet = 1;

constexpr int kNotFound = -1;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A dialogue text read by value from configuration.
inline HxStr ConfigText(const char *pszKey) {
    HxStr value;
    QueryConfigString(&value, kDialogueConfigCode, pszKey);
    return value;
}

// The no-space dialogue OnConnectState() and OnPersonasSaved() both expand.
inline void ShowNoSpace(MetScreen *pOwner, const HxStr &slotName, int nCopy) {
    std::vector<HxStr> buttons;
    if (nCopy == 0) {
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        HxStr format;
        QueryConfigString(&format, kDialogueConfigCode, kSaveNoSpaceText);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slotName)));
        MetMsgScreen::ShowActive(
            HxStr(kSaveNoSpaceDialogue), HxStr(kWarningTitle), text, kTwoButtons, buttons, pOwner);
    } else {
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kCancelButton));
        HxStr format;
        QueryConfigString(&format, kDialogueConfigCode, kCopyNoSpaceText);
        HxStr text(FormatString(
            TextOrEmpty(format), TextOrEmpty(slotName), GlobalSettings::shared()->mUnknown74));
        MetMsgScreen::ShowActive(
            HxStr(kCopyNoSpaceDialogue), HxStr(kWarningTitle), text, kTwoButtons, buttons, pOwner);
    }
}

// The no-card dialogue OnConnectState() and OnPersonasSaved() both expand.
inline void ShowNoCard(MetScreen *pOwner, const HxStr &slotName, int nDelete, int nCopy) {
    std::vector<HxStr> buttons;
    const char *pszKey;
    if (nDelete != 0) {
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        pszKey = kDeleteFailNoCardText;
    } else if (nCopy == 0) {
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        pszKey = kSaveFailNoCardText;
    } else {
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kCancelButton));
        pszKey = kCopyFailNoCardText;
    }
    HxStr format;
    QueryConfigString(&format, kDialogueConfigCode, pszKey);
    HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slotName)));
    MetMsgScreen::Show(
        HxStr(kMemCheckDialogue), HxStr(kErrorTitle), text, kTwoButtons, buttons, pOwner);
}

// The FreQ name keyboard OnMsgScreenDismissed() opens from two dialogues.
inline void OpenNameKeyboard(MetPersonaData *pPersona, MetKBUser *pUser) {
    MetKeyboardRequest request(HxStr(kOwnScreenName),
                               HxStr(kNamePrompt),
                               pPersona->mUnknown140.mUnknown00,
                               kAnyPad,
                               pUser);
    request.mMaxLength = kNameMaxLength;
    request.mMaxWidth = kNameMaxWidth;
    request.mTicker = kNoText;
    MetKeyboardScreen::Open(request);
}

} // namespace

// 0x0032e858
void MetPersonaSaverScreen::StartSave(const std::vector<HxStr> &screens,
                                      MetPersonaData *pPersona,
                                      const MemcardConnectState &slot,
                                      int nUnknown9c,
                                      int nUnknown98) {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    MetPersonaSaverScreen *pSaver =
        pScreen != nullptr ? dynamic_cast<MetPersonaSaverScreen *>(pScreen) : nullptr;
    // The binary does not test the result for null.
    pSaver->SetSaveRequest(screens, pPersona, slot);
    pSaver->mUnknown98 = nUnknown98;
    pSaver->mUnknown9c = nUnknown9c;
    pSaver->mUnknown94 = 0;

    MetScreen *pLoadGame = MetScreen::FindScreenByName(HxStr(kLoadGameScreen));
    pLoadGame->PushNamedScreen(HxStr(kOwnScreenName));
    pLoadGame->ActivateNamedPanel(HxStr(kOwnScreenName));
}

// 0x0032eaa8
void MetPersonaSaverScreen::StartDelete(const std::vector<HxStr> &screens,
                                        MetPersonaData *pPersona,
                                        const MemcardConnectState &slot) {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    MetPersonaSaverScreen *pSaver =
        pScreen != nullptr ? dynamic_cast<MetPersonaSaverScreen *>(pScreen) : nullptr;
    // The binary does not test the result for null.
    pSaver->SetSaveRequest(screens, pPersona, slot);
    pSaver->mUnknown9c = 0;
    pSaver->mUnknown98 = 0;
    pSaver->mUnknown94 = kDeleteRequest;

    MetScreen *pLoadGame = MetScreen::FindScreenByName(HxStr(kLoadGameScreen));
    pLoadGame->PushNamedScreen(HxStr(kOwnScreenName));
    pLoadGame->ActivateNamedPanel(HxStr(kOwnScreenName));
}

// 0x0032ece0
MetPersonaSaverScreen::MetPersonaSaverScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mUnknown98(0), mUnknown9c(0), mUnknownbc(0) {
}

// 0x0032f020
MetPersonaSaverScreen::~MetPersonaSaverScreen() {
    ClearPersonas();
}

// 0x0032f1e0
void MetPersonaSaverScreen::CommitSave() {
    if (mUnknown98 != 0 || mUnknownc0.mPortSlot != 0 ||
        MetFrontEndState::shared()->mUnknown0c != 0 || mUnknownb8 == nullptr) {
        MemcardManager::shared()->mUser = this;
        MemcardManager::shared()->CreateGetConnectStateTask(mUnknownc0.mPortSlot);
        return;
    }

    bool bFound = false;
    if (mUnknownb8->mUnknown15c != 0) {
        for (std::vector<MetPersonaData *>::size_type i = 0;
             i < MetFreqMakerAssetManager::shared()->GetIdentityList()->size();
             ++i) {
            HxStr name((*MetFreqMakerAssetManager::shared()->GetIdentityList())[i]
                           ->mUnknown140.mUnknown00);
            if (name == mUnknownb8->mUnknown140.mUnknown00) {
                bFound = true;
                *(*MetFreqMakerAssetManager::shared()->GetIdentityList())[i] = *mUnknownb8;
                break;
            }
        }
    }

    if (!bFound) {
        for (std::vector<MetPersonaData *>::size_type i = 0;
             i < MetPersonaData::savedList()->size();
             ++i) {
            if ((*MetPersonaData::savedList())[i]->mUnknown140.mUnknown00 ==
                mUnknownb8->mUnknown140.mUnknown00) {
                bFound = true;
                if (mUnknownb8 != (*MetPersonaData::savedList())[i]) {
                    *(*MetPersonaData::savedList())[i] = *mUnknownb8;
                }
                break;
            }
        }
    }

    if (!bFound) {
        MetPersonaData *pCopy = new MetPersonaData;
        *pCopy = *mUnknownb8;
        MetPersonaData::savedList()->push_back(pCopy);
    }
    BeginExit();
}

// 0x0032f4d0
void MetPersonaSaverScreen::ClearPersonas() {
    // Yes, the binary re-reads the size on every iteration rather than caching it.
    for (unsigned index = 0; index < mPersonas.size(); ++index) {
        delete mPersonas[index];
    }
    mPersonas.clear();
}

// 0x0032f5a0
void MetPersonaSaverScreen::OnConnectState(MemcardConnectState state, int nStatus) {
    if (nStatus != kMemcardStatusOk) {
        ShowNoCard(this, mUnknownc0.mSlotName, mUnknown94, mUnknown98);
        return;
    }

    if (state.mFormatted == 0) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        HxStr format;
        HxStr text;
        if (mUnknown98 == 0) {
            format = ConfigText(kSaveFailFormatText);
        } else {
            format = ConfigText(kCopyFailFormatText);
        }
        text = FormatString(TextOrEmpty(format), TextOrEmpty(mUnknownc0.mSlotName));
        MetMsgScreen::Show(
            HxStr(kFormatCheckDialogue), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
        return;
    }

    if (mUnknownbc != 0) {
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        GlobalSettings::shared()->mCardSlots[0] = state;
        mUnknownbc = 0;
        ExitScreenByName(HxStr(kMsgScreen));
        return;
    }

    if (mUnknown94 == 0 && state.mFree < GlobalSettings::shared()->mUnknown74) {
        ShowNoSpace(this, mUnknownc0.mSlotName, mUnknown98);
        return;
    }

    MemcardManager::shared()->mUser = this;
    ClearPersonas();
    MemcardManager::shared()->CreateLoadPersonasTask(mUnknownc0.mPortSlot, &mPersonas);

    std::vector<HxStr> buttons;
    HxStr format;
    HxStr text;
    HxStr title;
    if (mUnknown98 == 0) {
        title = ConfigText(kSaveTitleKey);
        format = ConfigText(kSaveText);
        text = FormatString(TextOrEmpty(format), TextOrEmpty(state.mSlotName));
        MetMsgScreen::Show(HxStr(kSaveDialogue), title, text, kNoButtons, buttons, this);
    } else if (mUnknown94 != 0) {
        std::vector<HxStr> noButtons;
        HxStr first;
        QueryConfigString(&first, kDialogueConfigCode, kDeleteFirstText);
        HxStr second;
        QueryConfigString(&second, kDialogueConfigCode, kDeleteSecondText);
        HxStr deleting(FormatString(kDeleteFormat,
                                    TextOrEmpty(first),
                                    TextOrEmpty(mUnknownc0.mSlotName),
                                    TextOrEmpty(second)));
        MetMsgScreen::Show(
            HxStr(kDeletingDialogue), HxStr(kWarningTitle), deleting, kNoButtons, noButtons, this);
    } else {
        title = ConfigText(kCopyTitleKey);
        format = ConfigText(kCopyText);
        text = FormatString(TextOrEmpty(format),
                            TextOrEmpty(NextCardSlot(state).mSlotName),
                            TextOrEmpty(state.mSlotName));
        MetMsgScreen::Show(HxStr(kSaveDialogue), title, text, kNoButtons, buttons, this);
    }
}

// 0x00331358
void MetPersonaSaverScreen::OnCardFormatted(int, int nStatus) {
    std::vector<HxStr> buttons;
    switch (nStatus) {
    case kMemcardStatusOk:
    case kMemcardStatusAlreadyFormatted: {
        buttons.push_back(HxStr(kContinueButton));
        HxStr format;
        QueryConfigString(&format,
                          kDialogueConfigCode,
                          nStatus == kMemcardStatusOk ? kFormatSuccessText : kFormatAlreadyText);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknownc0.mSlotName)));
        MetMsgScreen::ShowActive(
            HxStr(kFormatDoneDialogue), HxStr(kWarningTitle), text, kOneButton, buttons, this);
        break;
    }
    default: {
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kBackButton));
        HxStr name(kFormatFailDialogue);
        HxStr title(kErrorTitle);
        HxStr text;
        QueryConfigString(&text, kDialogueConfigCode, kFormatFailText);
        MetMsgScreen::Show(name, title, text, kTwoButtons, buttons, this);
        break;
    }
    }
}

// 0x00331d48
int MetPersonaSaverScreen::CheckPersonaLimit() {
    if (mPersonas.size() < static_cast<unsigned>(kMaxPersonas)) {
        return 1;
    }

    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kOkButton));
    HxStr format;
    QueryConfigString(&format, kDialogueConfigCode, kLimitText);
    HxStr text(FormatString(TextOrEmpty(format), kMaxPersonas, TextOrEmpty(mUnknownc0.mSlotName)));
    MetMsgScreen::Show(HxStr(kLimitDialogue), HxStr(kErrorTitle), text, kOneButton, buttons, this);
    return 0;
}

// 0x00332130
void MetPersonaSaverScreen::SyncActivePersona() {
    std::vector<MetPersonaData *> roster(*Application::shared()->GetGameManager()->GetPersonas());
    MetPersonaData *pActive = roster.size() != 0 ? roster[0] : nullptr;
    if (pActive != nullptr) {
        *pActive = *mUnknownb8;
    }
}

// 0x00332428
void MetPersonaSaverScreen::OnPersonasLoaded(int, int) {
    MetPersonaData *pPersona = mUnknownb8;
    if (pPersona->mUnknown140.mUnknown00 == kNoText) {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kOkButton));
        HxStr text;
        QueryConfigString(&text, kDialogueConfigCode, kNoNameText);
        MetMsgScreen::Show(
            HxStr(kNameRequiredDialogue), HxStr(kErrorTitle), text, kOneButton, buttons, this);
        return;
    }

    if (mUnknown98 != 0) {
        mUnknownb8->mUnknown160 = mUnknownb8->mUnknown140.mUnknown00;
    }
    int bRenamed = 0;
    if (!(mUnknownb8->mUnknown160 == kNoText)) {
        bRenamed = mUnknownb8->mUnknown160 != mUnknownb8->mUnknown140.mUnknown00;
    }

    // The entry saved under the persona's previous name, and the entry already using its name.
    int nOldIndex = kNotFound;
    int nNameIndex = kNotFound;
    for (std::vector<MetPersonaData *>::size_type i = 0; i < mPersonas.size(); ++i) {
        // Yes, the binary discards this comparison.
        (void)(mPersonas[i]->mUnknown160 == mPersonas[i]->mUnknown140.mUnknown00);
        if (mPersonas[i]->mUnknown160 == mUnknownb8->mUnknown160) {
            nOldIndex = i;
        }
        if (mPersonas[i]->mUnknown160 == mUnknownb8->mUnknown140.mUnknown00) {
            nNameIndex = i;
        }
    }

    if (mUnknown94 != 0) {
        if (nOldIndex == kNotFound) {
            std::vector<HxStr> buttons;
            buttons.push_back(HxStr(kRetryButton));
            buttons.push_back(HxStr(kContinueButton));
            HxStr format;
            QueryConfigString(&format, kDialogueConfigCode, kDeleteNotFoundText);
            HxStr text(FormatString(TextOrEmpty(format),
                                    TextOrEmpty(mUnknownb8->mUnknown140.mUnknown00),
                                    TextOrEmpty(mUnknownc0.mSlotName)));
            MetMsgScreen::Show(
                HxStr(kMemCheckDialogue), HxStr(kErrorTitle), text, kTwoButtons, buttons, this);
            return;
        }
        delete mPersonas[nOldIndex];
        mPersonas.erase(mPersonas.begin() + nOldIndex);
    }

    if (bRenamed != 0) {
        if (nNameIndex != kNotFound) {
            std::vector<HxStr> buttons;
            buttons.push_back(HxStr(kOkButton));
            HxStr format;
            QueryConfigString(&format, kDialogueConfigCode, kNewNameText);
            HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknownc0.mSlotName)));
            MetMsgScreen::Show(
                HxStr(kNameRequiredDialogue), HxStr(kErrorTitle), text, kOneButton, buttons, this);
            return;
        }
        if (nOldIndex != nNameIndex) {
            mUnknownb8->mUnknown160 = mUnknownb8->mUnknown140.mUnknown00;
            *mPersonas[nOldIndex] = *mUnknownb8;
            SyncActivePersona();
        } else {
            if (CheckPersonaLimit() == 0) {
                return;
            }
            mUnknownb8->mUnknown160 = mUnknownb8->mUnknown140.mUnknown00;
            MetPersonaData *pCopy = new MetPersonaData;
            *pCopy = *mUnknownb8;
            mPersonas.push_back(pCopy);
        }
    } else if (nNameIndex != kNotFound) {
        if (mUnknown9c != 0) {
            AskToReplace();
            return;
        }
        *mPersonas[nNameIndex] = *mUnknownb8;
        SyncActivePersona();
    } else {
        if (CheckPersonaLimit() == 0) {
            return;
        }
        mUnknownb8->mUnknown160 = mUnknownb8->mUnknown140.mUnknown00;
        MetPersonaData *pCopy = new MetPersonaData;
        *pCopy = *mUnknownb8;
        mPersonas.push_back(pCopy);
    }

    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateSavePersonasTask(mUnknownc0.mPortSlot, mPersonas);
}

// 0x00333210
void MetPersonaSaverScreen::OnPersonasSaved(int nPortSlot, int nStatus) {
    switch (nStatus) {
    case kMemcardStatusOk:
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        if (nPortSlot != GlobalSettings::shared()->mCardSlots[0].mPortSlot) {
            ExitScreenByName(HxStr(kMsgScreen));
            break;
        }
        MetPersonaData::ClearLoadList();
        for (std::vector<MetPersonaData *>::size_type i = 0; i < mPersonas.size(); ++i) {
            MetPersonaData *pCopy = new MetPersonaData;
            *pCopy = *mPersonas[i];
            MetPersonaData::loadList()->push_back(pCopy);
        }
        mUnknownbc = kFlagSet;
        MemcardManager::shared()->mUser = this;
        MemcardManager::shared()->CreateGetConnectStateTask(mUnknownc0.mPortSlot);
        break;
    case kMemcardStatusCardFull:
        ShowNoSpace(this, mUnknownc0.mSlotName, mUnknown98);
        break;
    case kMemcardStatusUnknown:
        ShowNoCard(this, mUnknownc0.mSlotName, mUnknown94, mUnknown98);
        break;
    default: {
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kRetryButton));
        buttons.push_back(HxStr(kContinueButton));
        HxStr name(kSaveNoSpaceDialogue);
        HxStr title(kWarningTitle);
        HxStr text;
        QueryConfigString(&text, kDialogueConfigCode, kSaveFailGeneralText);
        MetMsgScreen::ShowActive(name, title, text, kTwoButtons, buttons, this);
        break;
    }
    }
}

// 0x003346c8
void MetPersonaSaverScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kMemCheckDialogue) {
        if (nChoice == kChoiceSecond) {
            BeginExit();
        } else {
            CommitSave();
        }
    } else if (name == kFormatCheckDialogue) {
        if (nChoice != kChoiceSecond) {
            CommitSave();
            return;
        }
        MemcardManager::shared()->CreateFormatTask(mUnknownc0.mPortSlot);
        HxStr format;
        QueryConfigString(&format, kDialogueConfigCode, kFormatGoText);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknownc0.mSlotName)));
        std::vector<HxStr> noButtons;
        MetMsgScreen::Show(
            HxStr(kFormatGoDialogue), HxStr(kWarningTitle), text, kNoButtons, noButtons, this);
    } else if (name == kFormatDoneDialogue) {
        CommitSave();
    } else if (name == kFormatFailDialogue) {
        if (nChoice == kChoiceFirst) {
            CommitSave();
            return;
        }
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kContinueButton));
        HxStr warnName(kNoSaveWarnDialogue);
        HxStr title(kWarningTitle);
        HxStr text;
        QueryConfigString(&text, kDialogueConfigCode, kNoSaveWarnText);
        MetMsgScreen::ShowActive(warnName, title, text, kOneButton, buttons, this);
    } else if (name == kSaveNoSpaceDialogue || name == kCopyNoSpaceDialogue) {
        if (nChoice == kChoiceFirst) {
            CommitSave();
        } else {
            BeginExit();
        }
    } else if (name == kReplaceDialogue) {
        if (nChoice == kChoiceSecond) {
            mUnknown9c = 0;
            CommitSave();
        } else {
            OpenNameKeyboard(mUnknownb8, this);
        }
    } else if (name == kLimitDialogue) {
        BeginExit();
    } else if (name == kNameRequiredDialogue) {
        OpenNameKeyboard(mUnknownb8, this);
    } else {
        BeginExit();
    }
}

// 0x003350f8
void MetPersonaSaverScreen::AskToReplace() {
    HxStr format;
    QueryConfigString(&format, kDialogueConfigCode, kReplaceText);
    HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mUnknownc0.mSlotName)));
    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kNoButton));
    buttons.push_back(HxStr(kYesButton));
    MetMsgScreen::Show(
        HxStr(kReplaceDialogue), HxStr(kWarningTitle), text, kTwoButtons, buttons, this);
}

// 0x00338f98
MetPersonaSaverScreen *MetPersonaSaverScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPersonaSaverScreen(pRenderer, nPriority);
}

// 0x00339020
void MetPersonaSaverScreen::SetSaveRequest(const std::vector<HxStr> &screens,
                                           MetPersonaData *pPersona,
                                           const MemcardConnectState &slot) {
    mUnknownac = screens;
    mUnknownb8 = pPersona;
    mUnknownc0 = slot;
}

// 0x003390a0
void MetPersonaSaverScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
}

// 0x003390c0
void MetPersonaSaverScreen::EnterAndShow() {
    SetShowing(0); // Yes, the binary does not run MetScreen::EnterAndShow().
}

// 0x003390f0
void MetPersonaSaverScreen::OnUnknownSlot7() {
    mUnknownbc = 0;
    CommitSave();
}

// 0x00339110
void MetPersonaSaverScreen::BeginExit() {
    mUnknown10->RemoveScreen(this);
    const int nCount = mUnknownac.size();
    for (int i = 0; i < nCount; ++i) {
        PushNamedScreen(mUnknownac[i]);
    }
    ActivateNamedPanel(mUnknownac[0]);
}

// 0x003391a8
void MetPersonaSaverScreen::OnUnknownSlot2(const HxStr &text) {
    mUnknownb8->SetName(text);
}
