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
#include "met/metstrings.h"
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

// What StartDelete() records in mIsDelete.
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
#ifdef VIDEO_STANDARD_PAL
static const char *const kSaveNoFormatDialogue = "save_fail_no_format";
static const char *const kReplaceInLoadListDialogue = "freq_replace_catastrophe";
#endif

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

// The most personas one card stores. The `freq_limit` text also receives the figure.
constexpr int kMaxPersonas = 8;

// The FreQ name keyboard.
static const char *const kNamePrompt = "FreQ name";
static const char *const kNoText = "";
constexpr int kAnyPad = -1;
constexpr int kNameMaxLength = 12;
constexpr int kNameMaxWidth = 176;

// The value mIsDelete, mIsCopy, mConfirmReplace, and mRefreshingSettingsCard take while set.
constexpr int kFlagSet = 1;

constexpr int kNotFound = -1;

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A dialogue text read by value from configuration.
inline HxStr ConfigText(MetStringId nId, const char *pszKey) {
    HxStr value = MetConfigText(nId, kDialogueConfigCode, pszKey);
    return value;
}

#ifdef VIDEO_STANDARD_PAL
// The no-space dialogue OnPersonasSaved() expands. Both texts receive the kilobytes the card
// lacked.
inline void ShowNoSpace(MetScreen *pOwner, const HxStr &slotName, int nCopy, int nKilobytes) {
    std::vector<HxStr> buttons;
    if (nCopy == 0) {
        buttons.push_back(GetMetString(kMetStrMsgRETRY));
        buttons.push_back(GetMetString(kMetStrMsgCONTINUE));
        HxStr format = GetMetString(kMetStrSaveFailNospace);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slotName), nKilobytes));
        MetMsgScreen::ShowActive(HxStr(kSaveNoSpaceDialogue),
                                 GetMetString(kMetStrMsgWARNING),
                                 text,
                                 kTwoButtons,
                                 buttons,
                                 pOwner);
    } else {
        buttons.push_back(GetMetString(kMetStrMsgRETRY));
        buttons.push_back(GetMetString(kMetStrMsgCANCEL));
        HxStr format = GetMetString(kMetStrCopyFailNospace);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slotName), nKilobytes));
        MetMsgScreen::ShowActive(HxStr(kCopyNoSpaceDialogue),
                                 GetMetString(kMetStrMsgWARNING),
                                 text,
                                 kTwoButtons,
                                 buttons,
                                 pOwner);
    }
}
#else
// The no-space dialogue OnConnectState() and OnPersonasSaved() both expand.
inline void ShowNoSpace(MetScreen *pOwner, const HxStr &slotName, int nCopy) {
    std::vector<HxStr> buttons;
    if (nCopy == 0) {
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        HxStr format = MetConfigText(kMetStrSaveFailNospace, kDialogueConfigCode, kSaveNoSpaceText);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slotName)));
        MetMsgScreen::ShowActive(HxStr(kSaveNoSpaceDialogue),
                                 MetText(kMetStrMsgWARNING, kWarningTitle),
                                 text,
                                 kTwoButtons,
                                 buttons,
                                 pOwner);
    } else {
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCANCEL, kCancelButton));
        HxStr format = MetConfigText(kMetStrCopyFailNospace, kDialogueConfigCode, kCopyNoSpaceText);
        HxStr text(FormatString(TextOrEmpty(format),
                                TextOrEmpty(slotName),
                                GlobalSettings::shared()->mPersonaMinimumFreeClusters));
        MetMsgScreen::ShowActive(HxStr(kCopyNoSpaceDialogue),
                                 MetText(kMetStrMsgWARNING, kWarningTitle),
                                 text,
                                 kTwoButtons,
                                 buttons,
                                 pOwner);
    }
}
#endif

// The no-card dialogue OnConnectState() and OnPersonasSaved() both expand.
inline void ShowNoCard(MetScreen *pOwner, const HxStr &slotName, int nDelete, int nCopy) {
    std::vector<HxStr> buttons;
    if (nDelete != 0) {
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        HxStr format =
            MetConfigText(kMetStrDelFailNocard, kDialogueConfigCode, kDeleteFailNoCardText);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slotName)));
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           pOwner);
    } else if (nCopy == 0) {
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        HxStr format =
            MetConfigText(kMetStrSaveFailNocard, kDialogueConfigCode, kSaveFailNoCardText);
#ifdef VIDEO_STANDARD_PAL
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(FirstCardSlotName())));
#else
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slotName)));
#endif
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           pOwner);
    } else {
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCANCEL, kCancelButton));
        HxStr format =
            MetConfigText(kMetStrCopyFailNocard, kDialogueConfigCode, kCopyFailNoCardText);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(slotName)));
        MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           pOwner);
    }
}

// The FreQ name keyboard OnMsgScreenDismissed() opens from two dialogues.
inline void OpenNameKeyboard(MetPersonaData *pPersona, MetKBUser *pUser) {
    MetKeyboardRequest request(HxStr(kOwnScreenName),
                               MetText(kMetStrTMetKbFreq, kNamePrompt),
                               pPersona->mAppearance.mUserName,
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
                                      int nConfirmReplace,
                                      int nIsCopy) {
    MetScreen *pScreen = MetScreen::FindScreenByName(HxStr(kOwnScreenName));
    MetPersonaSaverScreen *pSaver =
        pScreen != nullptr ? dynamic_cast<MetPersonaSaverScreen *>(pScreen) : nullptr;
    // The binary does not test the result for null.
    pSaver->SetSaveRequest(screens, pPersona, slot);
    pSaver->mIsCopy = nIsCopy;
    pSaver->mConfirmReplace = nConfirmReplace;
    pSaver->mIsDelete = 0;

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
    pSaver->mConfirmReplace = 0;
    pSaver->mIsCopy = 0;
    pSaver->mIsDelete = kDeleteRequest;

    MetScreen *pLoadGame = MetScreen::FindScreenByName(HxStr(kLoadGameScreen));
    pLoadGame->PushNamedScreen(HxStr(kOwnScreenName));
    pLoadGame->ActivateNamedPanel(HxStr(kOwnScreenName));
}

// 0x0032ece0
MetPersonaSaverScreen::MetPersonaSaverScreen(MetRenderer *pRenderer, int nPriority)
    : MetScreen(pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mIsCopy(0), mConfirmReplace(0), mRefreshingSettingsCard(0) {
}

// 0x0032f020
MetPersonaSaverScreen::~MetPersonaSaverScreen() {
    ClearPersonas();
}

// 0x0032f1e0
void MetPersonaSaverScreen::CommitSave() {
    if (mIsCopy != 0 || mTargetSlot.mPortSlot != 0 ||
        MetFrontEndState::shared()->mUsingMemcard != 0 || mPersona == nullptr) {
        MemcardManager::shared()->mUser = this;
        MemcardManager::shared()->CreateGetConnectStateTask(mTargetSlot.mPortSlot);
        return;
    }

    bool bFound = false;
    if (mPersona->mIsPrefab != 0) {
        for (std::vector<MetPersonaData *>::size_type i = 0;
             i < MetFreqMakerAssetManager::shared()->GetIdentityList()->size();
             ++i) {
            HxStr name(
                (*MetFreqMakerAssetManager::shared()->GetIdentityList())[i]->mAppearance.mUserName);
            if (name == mPersona->mAppearance.mUserName) {
                bFound = true;
                *(*MetFreqMakerAssetManager::shared()->GetIdentityList())[i] = *mPersona;
                break;
            }
        }
    }

    if (!bFound) {
        for (std::vector<MetPersonaData *>::size_type i = 0;
             i < MetPersonaData::savedList()->size();
             ++i) {
            if ((*MetPersonaData::savedList())[i]->mAppearance.mUserName ==
                mPersona->mAppearance.mUserName) {
                bFound = true;
                if (mPersona != (*MetPersonaData::savedList())[i]) {
                    *(*MetPersonaData::savedList())[i] = *mPersona;
                }
                break;
            }
        }
    }

    if (!bFound) {
        MetPersonaData *pCopy = new MetPersonaData;
        *pCopy = *mPersona;
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

// NTSC-U/C: 0x0032f5a0, PAL: 0x00357ca8
void MetPersonaSaverScreen::OnConnectState(MemcardConnectState state, int nStatus) {
#ifdef VIDEO_STANDARD_PAL
    // The European release offers to format a card that reports itself unformatted.
    if (nStatus != kMemcardStatusOk && nStatus != kMemcardStatusNotFormatted) {
#else
    if (nStatus != kMemcardStatusOk) {
#endif
        ShowNoCard(this, mTargetSlot.mSlotName, mIsDelete, mIsCopy);
        return;
    }

#ifdef VIDEO_STANDARD_PAL
    if (nStatus == kMemcardStatusNotFormatted || state.mFormatted == 0) {
#else
    if (state.mFormatted == 0) {
#endif
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
        buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
        HxStr format;
        HxStr text;
        if (mIsCopy == 0) {
            format = ConfigText(kMetStrSaveFailFormat, kSaveFailFormatText);
        } else {
            format = ConfigText(kMetStrCopyFailFormat, kCopyFailFormatText);
        }
        text = FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName));
        MetMsgScreen::Show(HxStr(kFormatCheckDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        return;
    }

    if (mRefreshingSettingsCard != 0) {
        GlobalSettings::shared(); // Yes, the binary discards this call's result.
        GlobalSettings::shared()->mCardSlots[0] = state;
        mRefreshingSettingsCard = 0;
        ExitScreenByName(HxStr(kMsgScreen));
        return;
    }

#ifndef VIDEO_STANDARD_PAL
    if (mIsDelete == 0 && state.mFree < GlobalSettings::shared()->mPersonaMinimumFreeClusters) {
        ShowNoSpace(this, mTargetSlot.mSlotName, mIsCopy);
        return;
    }
#endif

    MemcardManager::shared()->mUser = this;
    ClearPersonas();
    MemcardManager::shared()->CreateLoadPersonasTask(mTargetSlot.mPortSlot, &mPersonas);

    std::vector<HxStr> buttons;
    HxStr format;
    HxStr text;
    HxStr title;
#ifdef VIDEO_STANDARD_PAL
    // The European release tests mIsCopy first and shows the deleting dialogue for a delete.
    const bool bSave = mIsCopy == 0 && mIsDelete == 0;
    const bool bDelete = mIsCopy == 0 && mIsDelete != 0;
#else
    const bool bSave = mIsCopy == 0;
    const bool bDelete = mIsDelete != 0;
#endif
    if (bSave) {
        title = ConfigText(kMetStrSaveTitle, kSaveTitleKey);
        format = ConfigText(kMetStrMemSave, kSaveText);
        text = FormatString(TextOrEmpty(format), TextOrEmpty(state.mSlotName));
        MetMsgScreen::Show(HxStr(kSaveDialogue), title, text, kNoButtons, buttons, this);
    } else if (bDelete) {
        std::vector<HxStr> noButtons;
        HxStr first = MetConfigText(kMetStrFreqMidDel1, kDialogueConfigCode, kDeleteFirstText);
        HxStr second = MetConfigText(kMetStrFreqMidDel2, kDialogueConfigCode, kDeleteSecondText);
        HxStr deleting(FormatString(kDeleteFormat,
                                    TextOrEmpty(first),
                                    TextOrEmpty(mTargetSlot.mSlotName),
                                    TextOrEmpty(second)));
        MetMsgScreen::Show(HxStr(kDeletingDialogue),
                           MetText(kMetStrMsgWARNING, kWarningTitle),
                           deleting,
                           kNoButtons,
                           noButtons,
                           this);
    } else {
        title = ConfigText(kMetStrCopyTitle, kCopyTitleKey);
        format = ConfigText(kMetStrMemCopy12, kCopyText);
        MemcardConnectState next = NextCardSlot(state);
        text = FormatString(
            TextOrEmpty(format), TextOrEmpty(next.mSlotName), TextOrEmpty(state.mSlotName));
        MetMsgScreen::Show(HxStr(kSaveDialogue), title, text, kNoButtons, buttons, this);
    }
}

// NTSC-U/C: 0x00331358, PAL: 0x00359ed0
void MetPersonaSaverScreen::OnCardFormatted(int, int nStatus) {
    std::vector<HxStr> buttons;
    switch (nStatus) {
    case kMemcardStatusOk:
    case kMemcardStatusAlreadyFormatted: {
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
#ifdef VIDEO_STANDARD_PAL
        HxStr format =
            GetMetString(nStatus == kMemcardStatusOk ? kMetStrFormatSuccess : kMetStrFormatAlready);
#else
        HxStr format = QueryConfigString(kDialogueConfigCode,
                                         nStatus == kMemcardStatusOk ? kFormatSuccessText :
                                                                       kFormatAlreadyText);
#endif
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName)));
        MetMsgScreen::ShowActive(HxStr(kFormatDoneDialogue),
                                 MetText(kMetStrMsgWARNING, kWarningTitle),
                                 text,
                                 kOneButton,
                                 buttons,
                                 this);
        break;
    }
    default: {
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgBACK, kBackButton));
        HxStr name(kFormatFailDialogue);
        HxStr title(MetText(kMetStrMsgERROR, kErrorTitle));
        HxStr text = MetConfigText(kMetStrFormatFail, kDialogueConfigCode, kFormatFailText);
        MetMsgScreen::Show(name, title, text, kTwoButtons, buttons, this);
        break;
    }
    }
}

// NTSC-U/C: 0x00331d48, PAL: 0x0035aa48
int MetPersonaSaverScreen::CheckPersonaLimit() {
    if (mPersonas.size() < static_cast<unsigned>(kMaxPersonas)) {
        return 1;
    }

    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
    HxStr format = MetConfigText(kMetStrFreqLimit, kDialogueConfigCode, kLimitText);
    HxStr text(FormatString(TextOrEmpty(format), kMaxPersonas, TextOrEmpty(mTargetSlot.mSlotName)));
    MetMsgScreen::Show(HxStr(kLimitDialogue),
                       MetText(kMetStrMsgERROR, kErrorTitle),
                       text,
                       kOneButton,
                       buttons,
                       this);
    return 0;
}

// 0x00332130
void MetPersonaSaverScreen::SyncActivePersona() {
    std::vector<MetPersonaData *> roster(*Application::shared()->GetGameManager()->GetPersonas());
    MetPersonaData *pActive = roster.size() != 0 ? roster[0] : nullptr;
    if (pActive != nullptr) {
        *pActive = *mPersona;
    }
}

// NTSC-U/C: 0x00332428, PAL: 0x0035b1a8
void MetPersonaSaverScreen::OnPersonasLoaded(int, int) {
    MetPersonaData *pPersona = mPersona;
    if (pPersona->mAppearance.mUserName == kNoText) {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
        HxStr text = MetConfigText(kMetStrFreqNoName, kDialogueConfigCode, kNoNameText);
        MetMsgScreen::Show(HxStr(kNameRequiredDialogue),
                           MetText(kMetStrMsgERROR, kErrorTitle),
                           text,
                           kOneButton,
                           buttons,
                           this);
        return;
    }

    if (mIsCopy != 0) {
        mPersona->mSavedName = mPersona->mAppearance.mUserName;
    }
    int bRenamed = 0;
    if (!(mPersona->mSavedName == kNoText)) {
        bRenamed = mPersona->mSavedName != mPersona->mAppearance.mUserName;
    }

    // The entry saved under the persona's previous name, and the entry already using its name.
    int nOldIndex = kNotFound;
    int nNameIndex = kNotFound;
    for (std::vector<MetPersonaData *>::size_type i = 0; i < mPersonas.size(); ++i) {
        // Yes, the binary discards this comparison.
        (void)(mPersonas[i]->mSavedName == mPersonas[i]->mAppearance.mUserName);
        if (mPersonas[i]->mSavedName == mPersona->mSavedName) {
            nOldIndex = i;
        }
        if (mPersonas[i]->mSavedName == mPersona->mAppearance.mUserName) {
            nNameIndex = i;
        }
    }

    if (mIsDelete != 0) {
        if (nOldIndex == kNotFound) {
            std::vector<HxStr> buttons;
            buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
            buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
            HxStr format =
                MetConfigText(kMetStrDelFailNotfound, kDialogueConfigCode, kDeleteNotFoundText);
            HxStr text(FormatString(TextOrEmpty(format),
                                    TextOrEmpty(mPersona->mAppearance.mUserName),
                                    TextOrEmpty(mTargetSlot.mSlotName)));
            MetMsgScreen::Show(HxStr(kMemCheckDialogue),
                               MetText(kMetStrMsgERROR, kErrorTitle),
                               text,
                               kTwoButtons,
                               buttons,
                               this);
            return;
        }
        delete mPersonas[nOldIndex];
        mPersonas.erase(mPersonas.begin() + nOldIndex);
    } else if (bRenamed != 0) {
        if (nNameIndex != kNotFound) {
            std::vector<HxStr> buttons;
            buttons.push_back(MetText(kMetStrMsgOK, kOkButton));
            HxStr format = MetConfigText(kMetStrFreqNewName, kDialogueConfigCode, kNewNameText);
            HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName)));
            MetMsgScreen::Show(HxStr(kNameRequiredDialogue),
                               MetText(kMetStrMsgERROR, kErrorTitle),
                               text,
                               kOneButton,
                               buttons,
                               this);
            return;
        }
        if (nOldIndex != nNameIndex) {
            mPersona->mSavedName = mPersona->mAppearance.mUserName;
            *mPersonas[nOldIndex] = *mPersona;
            SyncActivePersona();
        } else {
            if (CheckPersonaLimit() == 0) {
                return;
            }
            mPersona->mSavedName = mPersona->mAppearance.mUserName;
            MetPersonaData *pCopy = new MetPersonaData;
            *pCopy = *mPersona;
            mPersonas.push_back(pCopy);
        }
    } else if (nNameIndex != kNotFound) {
        if (mConfirmReplace != 0) {
#ifdef VIDEO_STANDARD_PAL
            AskToReplace(0);
#else
            AskToReplace();
#endif
            return;
        }
        *mPersonas[nNameIndex] = *mPersona;
        SyncActivePersona();
    } else {
        if (CheckPersonaLimit() == 0) {
            return;
        }
        mPersona->mSavedName = mPersona->mAppearance.mUserName;
        MetPersonaData *pCopy = new MetPersonaData;
        *pCopy = *mPersona;
        mPersonas.push_back(pCopy);
    }

    MemcardManager::shared()->mUser = this;
    MemcardManager::shared()->CreateSavePersonasTask(mTargetSlot.mPortSlot, mPersonas);
}

// NTSC-U/C: 0x00333210, PAL: 0x0035c120
#ifdef VIDEO_STANDARD_PAL
void MetPersonaSaverScreen::OnPersonasSaved(int nPortSlot, int nStatus, int nKilobytes) {
#else
void MetPersonaSaverScreen::OnPersonasSaved(int nPortSlot, int nStatus) {
#endif
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
        mRefreshingSettingsCard = kFlagSet;
        MemcardManager::shared()->mUser = this;
        MemcardManager::shared()->CreateGetConnectStateTask(mTargetSlot.mPortSlot);
        break;
    case kMemcardStatusCardFull:
#ifdef VIDEO_STANDARD_PAL
        ShowNoSpace(this, mTargetSlot.mSlotName, mIsCopy, nKilobytes);
#else
        ShowNoSpace(this, mTargetSlot.mSlotName, mIsCopy);
#endif
        break;
    case kMemcardStatusUnknown:
        ShowNoCard(this, mTargetSlot.mSlotName, mIsDelete, mIsCopy);
        break;
    default: {
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgRETRY, kRetryButton));
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        HxStr name(kSaveNoSpaceDialogue);
        HxStr title(MetText(kMetStrMsgWARNING, kWarningTitle));
        HxStr text =
            MetConfigText(kMetStrSaveFailGeneral, kDialogueConfigCode, kSaveFailGeneralText);
        MetMsgScreen::ShowActive(name, title, text, kTwoButtons, buttons, this);
        break;
    }
    }
}

// NTSC-U/C: 0x003346c8, PAL: 0x0035dbd8
void MetPersonaSaverScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kMemCheckDialogue) {
        if (nChoice == kChoiceSecond) {
#ifdef VIDEO_STANDARD_PAL
            LeaveWithoutSaving();
#else
            BeginExit();
#endif
        } else {
            CommitSave();
        }
    } else if (name == kFormatCheckDialogue) {
        if (nChoice != kChoiceSecond) {
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
            CommitSave();
#endif
            return;
        }
        MemcardManager::shared()->CreateFormatTask(mTargetSlot.mPortSlot);
        HxStr format = MetConfigText(kMetStrMemFormatGo, kDialogueConfigCode, kFormatGoText);
        HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName)));
        std::vector<HxStr> noButtons;
        MetMsgScreen::Show(HxStr(kFormatGoDialogue),
                           MetText(kMetStrMsgWARNING, kWarningTitle),
                           text,
                           kNoButtons,
                           noButtons,
                           this);
    } else if (name == kFormatDoneDialogue) {
        CommitSave();
    } else if (name == kFormatFailDialogue) {
        if (nChoice == kChoiceFirst) {
            CommitSave();
            return;
        }
        std::vector<HxStr> buttons;
        buttons.push_back(MetText(kMetStrMsgCONTINUE, kContinueButton));
        HxStr warnName(kNoSaveWarnDialogue);
        HxStr title(MetText(kMetStrMsgWARNING, kWarningTitle));
        HxStr text = MetConfigText(kMetStrNoSaveWarn, kDialogueConfigCode, kNoSaveWarnText);
        MetMsgScreen::ShowActive(warnName, title, text, kOneButton, buttons, this);
#ifdef VIDEO_STANDARD_PAL
    } else if (name == kSaveNoSpaceDialogue || name == kCopyNoSpaceDialogue ||
               name == kSaveNoFormatDialogue) {
        if (nChoice == kChoiceFirst) {
            CommitSave();
        } else {
            LeaveWithoutSaving();
        }
    } else if (name == kReplaceInLoadListDialogue) {
        if (nChoice == kChoiceSecond) {
            std::vector<MetPersonaData *>::size_type i = 0;
            while (i < MetPersonaData::loadList()->size() &&
                   !((*MetPersonaData::loadList())[i]->mAppearance.mUserName ==
                     mPersona->mAppearance.mUserName)) {
                ++i;
            }
            // The binary does not test for a missing entry. KeepInLoadList() raises the dialogue
            // only once it has found one.
            *(*MetPersonaData::loadList())[i] = *mPersona;
            BeginExit();
        } else {
            OpenNameKeyboard(mPersona, this);
        }
#else
    } else if (name == kSaveNoSpaceDialogue || name == kCopyNoSpaceDialogue) {
        if (nChoice == kChoiceFirst) {
            CommitSave();
        } else {
            BeginExit();
        }
#endif
    } else if (name == kReplaceDialogue) {
        if (nChoice == kChoiceSecond) {
            mConfirmReplace = 0;
            CommitSave();
        } else {
            OpenNameKeyboard(mPersona, this);
        }
    } else if (name == kLimitDialogue) {
        BeginExit();
    } else if (name == kNameRequiredDialogue) {
        OpenNameKeyboard(mPersona, this);
#ifdef VIDEO_STANDARD_PAL
    } else if (name == kNoSaveWarnDialogue) {
        LeaveWithoutSaving();
#endif
    } else {
        BeginExit();
    }
}

#ifdef VIDEO_STANDARD_PAL
void MetPersonaSaverScreen::LeaveWithoutSaving() {
    if (mConfirmReplace != 0 && mIsCopy == 0) {
        KeepInLoadList();
    } else {
        BeginExit();
    }
}

// PAL: 0x0035da70
void MetPersonaSaverScreen::KeepInLoadList() {
    int bFound = 0;
    for (std::vector<MetPersonaData *>::size_type i = 0; i < MetPersonaData::loadList()->size();
         ++i) {
        if ((*MetPersonaData::loadList())[i]->mAppearance.mUserName ==
            mPersona->mAppearance.mUserName) {
            bFound = kFlagSet;
            break;
        }
    }
    if (bFound != 0) {
        AskToReplace(kFlagSet);
        return;
    }

    MetPersonaData *pCopy = new MetPersonaData;
    *pCopy = *mPersona;
    MetPersonaData::loadList()->push_back(pCopy);
    BeginExit();
}
#endif

// NTSC-U/C: 0x003350f8, PAL: 0x0035ee80
#ifdef VIDEO_STANDARD_PAL
void MetPersonaSaverScreen::AskToReplace(int bInLoadList) {
#else
void MetPersonaSaverScreen::AskToReplace() {
#endif
    HxStr format = MetConfigText(kMetStrFreqReplace, kDialogueConfigCode, kReplaceText);
    HxStr text(FormatString(TextOrEmpty(format), TextOrEmpty(mTargetSlot.mSlotName)));
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
    buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
#ifdef VIDEO_STANDARD_PAL
    if (bInLoadList != 0) {
        MetMsgScreen::Show(HxStr(kReplaceInLoadListDialogue),
                           GetMetString(kMetStrMsgWARNING),
                           text,
                           kTwoButtons,
                           buttons,
                           this);
        return;
    }
#endif
    MetMsgScreen::Show(HxStr(kReplaceDialogue),
                       MetText(kMetStrMsgWARNING, kWarningTitle),
                       text,
                       kTwoButtons,
                       buttons,
                       this);
}

// 0x00338f98
MetPersonaSaverScreen *MetPersonaSaverScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetPersonaSaverScreen(pRenderer, nPriority);
}

// 0x00339020
void MetPersonaSaverScreen::SetSaveRequest(const std::vector<HxStr> &screens,
                                           MetPersonaData *pPersona,
                                           const MemcardConnectState &slot) {
    mReturnScreens = screens;
    mPersona = pPersona;
    mTargetSlot = slot;
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
void MetPersonaSaverScreen::OnPanelActivated() {
    mRefreshingSettingsCard = 0;
    CommitSave();
}

// 0x00339110
void MetPersonaSaverScreen::BeginExit() {
    mRenderer->RemoveScreen(this);
    const int nCount = mReturnScreens.size();
    for (int i = 0; i < nCount; ++i) {
        PushNamedScreen(mReturnScreens[i]);
    }
    ActivateNamedPanel(mReturnScreens[0]);
}

// 0x003391a8
void MetPersonaSaverScreen::OnKeyboardTextEntered(const HxStr &text) {
    mPersona->SetName(text);
}
