#include "met/metsaveremixscreen.h"

#include <vector>

#include "app/application.h"
#include "app/playsound.h"
#include "game/freqappearance.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "memcard/memcardconnectstate.h"
#include "met/metbuttonlist.h"
#include "met/metfrontendstate.h"
#include "met/methelpscreen.h"
#include "met/metkeyboardrequest.h"
#include "met/metkeyboardscreen.h"
#include "met/metmsgscreen.h"
#include "met/metpersonadata.h"
#include "met/metremixmanager.h"
#include "met/metremixrecord.h"
#include "met/metremixsaver.h"
#include "met/metrenderer.h"
#include "met/metscreen.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "os/log.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "script/configquery.h"

namespace {

// The screen name, from which the two animation view names are formatted.
static const char *const kScreenName = "ers";
// The directory the container loads from.
static const char *const kDirectory = "metagame/_Solo";
// The container name, without its `.rnd` suffix.
static const char *const kContainerName = "save_remix";

// The one container object the screen registers.
static const char *const kSaveObjectName = "remix_save";

// The registry keys Open() looks up.
static const char *const kSaveRemixScreen = "MetSaveRemixScreen";
static const char *const kLoadGameScreen = "MetLoadGameScreen";
static const char *const kEmptyText = "";

// The arguments OnSaveAbandoned() and OnSaveDialogueClosed() pass to
// MetRemixSaver::OnSaveFinished(). Neither override reads them.
constexpr int kSaverAbandoned = 0;
constexpr int kSaverDialogueClosed = 1;

// The flag slots 7 and 15 pass to MetRemixSaver::SetOwnerScreenShowing() when the save screen
// returns.
constexpr int kSaverReturned = 1;

// The screens the save screen exits or brings back.
static const char *const kHelpScreen = "MetHelpScreen";
static const char *const kTitleScreen = "MetScreenTitleScreen";

// The objects ResolveContainerViews() resolves, and the one button it adds.
static const char *const kFreqNameTextObject = "ers_freqname_player.txt";
static const char *const kInstructionsTextObject = "ers_instructions.txt";
static const char *const kRemixNameTextObject = "ers_remix_title_val.txt";
static const char *const kSaveCopyButton = "save_copy.but";

// The dialogue OnMsgScreenDismissed() handles itself, and its accepting choice.
static const char *const kDiscardDialogue = "discard_remix";
constexpr int kChoiceDiscard = 1;

// What MetScreen::mExitChoice records for the exit hook to act on.
constexpr int kExitDiscard = 0;
constexpr int kExitToHelp = 2;

// The navigation codes above MetScreenCommandCode that only this screen acts on.
constexpr int kCommandOpenKeyboard = 7;
constexpr int kCommandDecline = 8;

// The selection alternation select starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// The flag HandleCommand() passes to MetRemixSaver::SetOwnerScreenShowing() when the save is
// declined, and the argument slot 36 passes to OnSaveFinished() after a back exit.
constexpr int kSaverDeclined = 0;
constexpr int kSaverBack = 0;

// The sound select and decline play.
static const char *const kToggleSound = "SND_MET_FM_TOGGLE";

// The keyboard request HandleCommand() and OnDuplicateNameDeclined() build. The two prompts differ
// in case.
static const char *const kCommandKeyboardPrompt = "Remix Name";
static const char *const kDuplicateKeyboardPrompt = "Remix name";
static const char *const kKeyboardTicker = "met_save_remix_screen_ticker_tape";
constexpr int kKeyboardMaxWidth = 228;
constexpr int kKeyboardMaxLength = 32;
constexpr int kKeyboardUnusedFlag = 1;
constexpr int kAnyPad = -1;

// Configuration codes and keys EnterAndShow() and slot 36 read.
constexpr int kDialogueConfigCode = 0x258;
constexpr int kDefaultNameConfigCode = 0x325;
constexpr int kShortNameConfigCode = 0x327;
constexpr int kAlbumNumberConfigCode = 0x514;
static const char *const kPlayerKey = "remix_player";
static const char *const kCommandKey = "save_remix_command";
static const char *const kDiscardChangesKey = "discard_remix_changes";

// Formats and objects EnterAndShow() uses.
static const char *const kPlayerFormat = "%s %d";
static const char *const kPersonaFormat = "%s:";
static const char *const kCommandFormat = "%s %s.";
static const char *const kNumberedNameFormat = "%s 01";
static const char *const kNameTooLongFormat = "%s is too long to fit in the box!\n";
static const char *const kPlayerPanelObject = "ers_player_pan.txt";
static const char *const kHelpPreset = "remix_save_options";

// The discard dialogue slot 36 raises.
static const char *const kWarningTitle = "WARNING";
static const char *const kNoButton = "NO";
static const char *const kYesButton = "YES";
constexpr int kTwoButtons = 2;

// The selection EnterAndShow() starts on.
constexpr int kFirstButtonIndex = 0;

inline Rnd::Text *FindText(const char *pszName) {
    return dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr(pszName)));
}

inline const char *TextOrEmpty(const HxStr &text) {
    return text.mStr != nullptr ? text.mStr : g_szEmptyString;
}

// A configuration string read by value, with one substituted argument.
inline HxStr ConfigText(int nCode, const char *pszArgument) {
    HxStr value = QueryConfigString(nCode, pszArgument);
    return value;
}

} // namespace

// 0x0037ace0
MetSaveRemixScreen::MetSaveRemixScreen(MetRenderer *pRenderer, int nPriority)
    : MetSaveRemix(
          pRenderer, nPriority, HxStr(kScreenName), HxStr(kDirectory), HxStr(kContainerName)),
      mButtonList(nullptr), mPersona(nullptr), mDeclined(0) {
    mButtonList = new MetButtonList;
    mHelpKeys.push_back(HxStr(kSaveObjectName));
    mPlaysCommandSounds = 0;
    mKeyboardPending = 0;
}

// 0x003817e0
MetSaveRemixScreen *MetSaveRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSaveRemixScreen(pRenderer, nPriority);
}

// 0x00381868
MetSaveRemixScreen::~MetSaveRemixScreen() {
    delete mButtonList;
}

// 0x0037a9e0
void MetSaveRemixScreen::Open(MetPersonaData *pPersona,
                              int nPad,
                              MetRemixSaver *pSaver,
                              const MemcardConnectState &slot,
                              const std::vector<FreqAppearance> &appearances,
                              int bClearName) {
    MetSaveRemixScreen *pScreen =
        dynamic_cast<MetSaveRemixScreen *>(MetScreen::FindScreenByName(HxStr(kSaveRemixScreen)));
    pScreen->SetPersona(pPersona);
    pScreen->SetOwnerPad(nPad);
    pScreen->SetSaver(pSaver);
    pScreen->SetAppearances(appearances);
    pScreen->mTargetSlot = slot;
    if (bClearName != 0) {
        pScreen->SetEnteredName(HxStr(kEmptyText));
    }
    MetScreen *pLoadGame = MetScreen::FindScreenByName(HxStr(kLoadGameScreen));
    pLoadGame->PushNamedScreen(HxStr(kSaveRemixScreen));
    pLoadGame->ActivateNamedPanel(HxStr(kSaveRemixScreen));
}

// 0x00381910
void MetSaveRemixScreen::SetPersona(MetPersonaData *pPersona) {
    mPersona = pPersona;
}

// 0x00381918
void MetSaveRemixScreen::SetOwnerPad(int nPad) {
    mOwnerPad = nPad;
}

// 0x00381920
void MetSaveRemixScreen::SetSaver(MetRemixSaver *pSaver) {
    mSaver = pSaver;
}

// 0x00381928
void MetSaveRemixScreen::SetAppearances(const std::vector<FreqAppearance> &appearances) {
    mAppearances = appearances;
}

// 0x00381948
void MetSaveRemixScreen::SetEnteredName(const HxStr &text) {
    mEnteredName = text;
}

// 0x00381968
void MetSaveRemixScreen::PlaySlideSound(int nSelector) {
    if (nSelector == mOwnerPad) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// 0x00381990
void MetSaveRemixScreen::OnKeyboardTextEntered(const HxStr &text) {
    mRemixNameText->SetText(text); // The binary dereferences the text object with no null check.
    if (mKeyboardPending != 0) {
        MetSaveRemix::OnKeyboardTextEntered(text);
        mKeyboardPending = 0;
    }
}

// 0x003819f0
void MetSaveRemixScreen::OnSaveAbandoned() {
    if (mSaver != nullptr) {
        mSaver->OnSaveFinished(kSaverAbandoned);
    }
}

// 0x00381a28
void MetSaveRemixScreen::OnSaveDialogueClosed() {
    if (mSaver != nullptr) {
        mSaver->OnSaveFinished(kSaverDialogueClosed);
    }
}

// 0x0037af98
void MetSaveRemixScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mFreqNameText = FindText(kFreqNameTextObject);
    mInstructionsText = FindText(kInstructionsTextObject);
    mRemixNameText = FindText(kRemixNameTextObject);
    mButtonList->Add(HxStr(kSaveCopyButton), HxStr(kEmptyText));
}

// 0x0037c110
void MetSaveRemixScreen::OnRepeatingSoundFinished([[maybe_unused]] Rnd::Button *pButton) {
    mExitChoice = kExitToHelp;
    if (mSaver != nullptr) {
        mSaver->OnHelpRequested();
    }
    ExitScreenByName(HxStr(kHelpScreen));
    ExitScreenByName(HxStr(kTitleScreen));
    BeginExit();
}

// 0x0037cac8
void MetSaveRemixScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (!(name == kDiscardDialogue)) {
        MetSaveRemix::OnMsgScreenDismissed(name, nChoice);
        return;
    }
    if (nChoice == kChoiceDiscard) {
        mExitChoice = kExitDiscard;
        mDeclined = 0;
        OnExitFinished();
    } else {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kSaveRemixScreen));
        mSaver->SetOwnerScreenShowing(kSaverReturned);
        ActivateNamedPanel(HxStr(kSaveRemixScreen));
    }
}

// 0x0037b718
void MetSaveRemixScreen::OnPanelActivated() {
    if (mKeyboardPending == 0) {
        MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
        return;
    }
    mKeyboardPending = 0;
    if (mSaver != nullptr) {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kSaveRemixScreen));
        mSaver->SetOwnerScreenShowing(kSaverReturned);
        ActivateNamedPanel(HxStr(kSaveRemixScreen));
    }
}

// 0x0037b258
void MetSaveRemixScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (mOwnerPad != pCommand->mPadIndex) {
        return;
    }
    switch (pCommand->mCommand) {
    case kMetScreenCommandSelect:
        if (mRemixNameText->mPreWrapText.mLen != 0) {
            mEnteredName = mRemixNameText->mPreWrapText;
            ActivateNamedPanel(HxStr(kEmptyText));
            StartRepeatingSound(mRenderer->mAnimationFrame,
                                kSelectAlternateInterval,
                                mButtonList->mSelectedButton,
                                kSelectAlternateCycles);
            PlaySlideSound(pCommand->mPadIndex);
        } else {
            PlayErrorSound(mOwnerPad);
        }
        break;

    case kCommandOpenKeyboard: {
        MetKeyboardRequest request(HxStr(kSaveRemixScreen),
                                   HxStr(kCommandKeyboardPrompt),
                                   mRemixNameText->mPreWrapText,
                                   mOwnerPad,
                                   this);
        request.mUnusedFlag = kKeyboardUnusedFlag;
        request.mMaxWidth = kKeyboardMaxWidth;
        request.mMaxLength = kKeyboardMaxLength;
        request.mTicker = kKeyboardTicker;
        PlaySoundByName(kToggleSound);
        MetKeyboardScreen::Open(request);
        break;
    }

    case kCommandDecline:
        ActivateNamedPanel(HxStr(kEmptyText));
        PlaySoundByName(kToggleSound);
        mDeclined = 1;
        mSaver->SetOwnerScreenShowing(kSaverDeclined);
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x0037ccc8
void MetSaveRemixScreen::OnDuplicateNameDeclined() {
    mKeyboardPending = 1;
    MetKeyboardRequest request(
        HxStr(kSaveRemixScreen), HxStr(kDuplicateKeyboardPrompt), mRemixName, kAnyPad, this);
    request.mMaxWidth = kKeyboardMaxWidth;
    request.mMaxLength = kKeyboardMaxLength;
    request.mTicker = kKeyboardTicker;
    MetKeyboardScreen::Open(request);
}

// 0x0037b8e8
void MetSaveRemixScreen::EnterAndShow() {
    mCopying = 0;
    mKeyboardPending = 0;
    mRenderer->SetActivePanel(this);
    mRenderer->OnReturnFromGame();
    mButtonList->SetSelected(kFirstButtonIndex);

    const HxStr playerFormat(ConfigText(kDialogueConfigCode, kPlayerKey));
    const HxStr player(FormatString(kPlayerFormat, TextOrEmpty(playerFormat), mOwnerPad));
    FindText(kPlayerPanelObject)->SetText(player);

    if (mPersona == nullptr) {
        mPersona = MetFrontEndState::shared()->GetFirstPersona();
    }
    mFreqNameText->SetText(
        HxStr(FormatString(kPersonaFormat, TextOrEmpty(mPersona->mAppearance.mUserName))));

    const HxStr command(ConfigText(kDialogueConfigCode, kCommandKey));
    const HxStr instructions(
        FormatString(kCommandFormat, TextOrEmpty(command), TextOrEmpty(mTargetSlot.mSlotName)));
    mInstructionsText->SetText(instructions);

    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    HxStr name;
    if (mEnteredName != kEmptyText) {
        mRemixNameText->SetText(mEnteredName);
    } else if (params.mLoadingGame != 0) {
        MetRemixRecord record(*MetRemixManager::shared()->GetRecord());
        name = record.name;
        mRemixNameText->SetText(name);
    } else {
        HxStr numbered;
        name = ConfigText(kDefaultNameConfigCode, TextOrEmpty(params.mLevelName));
        numbered = FormatString(kNumberedNameFormat, TextOrEmpty(name));
        const float flWrapWidth = mRemixNameText->mWrapWidth;
        if (flWrapWidth < mRemixNameText->MeasureText(TextOrEmpty(numbered), numbered.mLen)) {
            name = ConfigText(kShortNameConfigCode, TextOrEmpty(params.mLevelName));
            numbered = FormatString(kNumberedNameFormat, TextOrEmpty(name));
            if (flWrapWidth < mRemixNameText->MeasureText(TextOrEmpty(numbered), numbered.mLen)) {
                Fatal(kNameTooLongFormat, TextOrEmpty(numbered));
            }
        }
        mRemixNameText->SetText(HxStr(TextOrEmpty(numbered)));
    }

    MetHelpScreen::SetText(mHelpKeys[0], mRenderer->mAnimationFrame);
    MetHelpScreen::SelectPreset(HxStr(kHelpPreset));
    MetScreen::EnterAndShow();
}

// 0x0037c260
void MetSaveRemixScreen::OnExitFinished() {
    if (mDeclined != 0) {
        mDeclined = 0;
        HxStr text;
        GameParams params(*Application::shared()->GetGameManager()->GetParams());
        if (params.mLoadingGame != 0) {
            text = ConfigText(kDialogueConfigCode, kDiscardChangesKey);
        } else {
            text = ConfigText(kDialogueConfigCode, kDiscardDialogue);
        }
        std::vector<HxStr> buttons;
        buttons.push_back(HxStr(kNoButton));
        buttons.push_back(HxStr(kYesButton));
        MetMsgScreen::Show(
            HxStr(kDiscardDialogue), HxStr(kWarningTitle), text, kTwoButtons, buttons, this);
        MetMsgScreen::SetOwnerPad(mOwnerPad);
        return;
    }

    if (mExitChoice == kExitDiscard) {
        if (mSaver != nullptr) {
            mSaver->OnSaveFinished(kSaverBack);
        }
        return;
    }

    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    RecordPendingSave(mTargetSlot,
                      mOwnerPad,
                      HxStr(TextOrEmpty(mRemixNameText->mPreWrapText)),
                      params.mLevelName,
                      mAppearances,
                      QueryConfigValue(kAlbumNumberConfigCode));
}
