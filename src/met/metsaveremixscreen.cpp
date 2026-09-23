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

// The arguments slots 40 and 41 pass to MetRemixSaver::OnUnknownSlot2(), which neither override
// reads.
constexpr int kSlot40SaverArgument = 0;
constexpr int kSlot41SaverArgument = 1;

// The flag slots 7 and 15 pass to MetRemixSaver::OnUnknownSlot4() when the save screen returns.
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

// What MetScreen::mUnknown18 records for the exit hook to act on.
constexpr int kExitDiscard = 0;
constexpr int kExitToHelp = 2;

// The navigation codes above MetScreenCommandCode that only this screen acts on.
constexpr int kCommandOpenKeyboard = 7;
constexpr int kCommandDecline = 8;

// The selection alternation select starts.
constexpr float kSelectAlternateInterval = 30.0f;
constexpr int kSelectAlternateCycles = 2;

// The flag HandleCommand() passes to MetRemixSaver::OnUnknownSlot4() when the save is declined,
// and the argument slot 36 passes to OnUnknownSlot2() after a back exit.
constexpr int kSaverDeclined = 0;
constexpr int kSaverBack = 0;

// The sound select and decline play.
static const char *const kToggleSound = "SND_MET_FM_TOGGLE";

// The keyboard request HandleCommand() and slot 42 build. The two prompts differ in case.
static const char *const kCommandKeyboardPrompt = "Remix Name";
static const char *const kSlot42KeyboardPrompt = "Remix name";
static const char *const kKeyboardTicker = "met_save_remix_screen_ticker_tape";
constexpr int kKeyboardMaxWidth = 228;
constexpr int kKeyboardMaxLength = 32;
constexpr int kKeyboardUnknown1c = 1;
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
      mUnknowne8(nullptr), mPersona(nullptr), mUnknown100(0) {
    mUnknowne8 = new MetButtonList;
    mUnknown38.push_back(HxStr(kSaveObjectName));
    mUnknown5c = 0;
    mUnknowne0 = 0;
}

// 0x003817e0
MetSaveRemixScreen *MetSaveRemixScreen::New(MetRenderer *pRenderer, int nPriority) {
    return new MetSaveRemixScreen(pRenderer, nPriority);
}

// 0x00381868
MetSaveRemixScreen::~MetSaveRemixScreen() {
    delete mUnknowne8;
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
    pScreen->mUnknown94 = slot;
    if (bClearName != 0) {
        pScreen->SetUnknown104(HxStr(kEmptyText));
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
    mUnknownc8 = nPad;
}

// 0x00381920
void MetSaveRemixScreen::SetSaver(MetRemixSaver *pSaver) {
    mUnknownfc = pSaver;
}

// 0x00381928
void MetSaveRemixScreen::SetAppearances(const std::vector<FreqAppearance> &appearances) {
    mUnknownac = appearances;
}

// 0x00381948
void MetSaveRemixScreen::SetUnknown104(const HxStr &text) {
    mUnknown104 = text;
}

// 0x00381968
void MetSaveRemixScreen::PlaySlideSound(int nSelector) {
    if (nSelector == mUnknownc8) {
        MetScreen::PlaySlideSound(nSelector);
    }
}

// 0x00381990
void MetSaveRemixScreen::OnUnknownSlot2(const HxStr &text) {
    mRemixNameText->SetText(text); // The binary dereferences the text object with no null check.
    if (mUnknowne0 != 0) {
        MetSaveRemix::OnUnknownSlot2(text);
        mUnknowne0 = 0;
    }
}

// 0x003819f0
void MetSaveRemixScreen::OnUnknownSlot40() {
    if (mUnknownfc != nullptr) {
        mUnknownfc->OnUnknownSlot2(kSlot40SaverArgument);
    }
}

// 0x00381a28
void MetSaveRemixScreen::OnUnknownSlot41() {
    if (mUnknownfc != nullptr) {
        mUnknownfc->OnUnknownSlot2(kSlot41SaverArgument);
    }
}

// 0x0037af98
void MetSaveRemixScreen::ResolveContainerViews() {
    MetScreen::ResolveContainerViews();
    mFreqNameText = FindText(kFreqNameTextObject);
    mInstructionsText = FindText(kInstructionsTextObject);
    mRemixNameText = FindText(kRemixNameTextObject);
    mUnknowne8->Add(HxStr(kSaveCopyButton), HxStr(kEmptyText));
}

// 0x0037c110
void MetSaveRemixScreen::OnUnknownSlot30([[maybe_unused]] Rnd::Button *pButton) {
    mUnknown18 = kExitToHelp;
    if (mUnknownfc != nullptr) {
        mUnknownfc->OnUnknownSlot3();
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
        mUnknown18 = kExitDiscard;
        mUnknown100 = 0;
        OnUnknownSlot36();
    } else {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kSaveRemixScreen));
        mUnknownfc->OnUnknownSlot4(kSaverReturned);
        ActivateNamedPanel(HxStr(kSaveRemixScreen));
    }
}

// 0x0037b718
void MetSaveRemixScreen::OnUnknownSlot7() {
    if (mUnknowne0 == 0) {
        MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
        return;
    }
    mUnknowne0 = 0;
    if (mUnknownfc != nullptr) {
        PushNamedScreen(HxStr(kHelpScreen));
        PushNamedScreen(HxStr(kSaveRemixScreen));
        mUnknownfc->OnUnknownSlot4(kSaverReturned);
        ActivateNamedPanel(HxStr(kSaveRemixScreen));
    }
}

// 0x0037b258
void MetSaveRemixScreen::HandleCommand(const MetScreenCommand *pCommand) {
    if (mUnknownc8 != pCommand->mPadIndex) {
        return;
    }
    switch (pCommand->mCommand) {
    case kMetScreenCommandSelect:
        if (mRemixNameText->mPreWrapText.mLen != 0) {
            mUnknown104 = mRemixNameText->mPreWrapText;
            ActivateNamedPanel(HxStr(kEmptyText));
            StartRepeatingSound(mUnknown10->mUnknown68,
                                kSelectAlternateInterval,
                                mUnknowne8->mUnknown00,
                                kSelectAlternateCycles);
            PlaySlideSound(pCommand->mPadIndex);
        } else {
            PlayErrorSound(mUnknownc8);
        }
        break;

    case kCommandOpenKeyboard: {
        MetKeyboardRequest request(HxStr(kSaveRemixScreen),
                                   HxStr(kCommandKeyboardPrompt),
                                   mRemixNameText->mPreWrapText,
                                   mUnknownc8,
                                   this);
        request.mUnknown1c = kKeyboardUnknown1c;
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
        mUnknown100 = 1;
        mUnknownfc->OnUnknownSlot4(kSaverDeclined);
        ExitScreenByName(HxStr(kHelpScreen));
        ExitScreenByName(HxStr(kTitleScreen));
        BeginExit();
        break;

    default:
        break;
    }
}

// 0x0037ccc8
void MetSaveRemixScreen::OnUnknownSlot42() {
    mUnknowne0 = 1;
    MetKeyboardRequest request(
        HxStr(kSaveRemixScreen), HxStr(kSlot42KeyboardPrompt), mUnknownb8, kAnyPad, this);
    request.mMaxWidth = kKeyboardMaxWidth;
    request.mMaxLength = kKeyboardMaxLength;
    request.mTicker = kKeyboardTicker;
    MetKeyboardScreen::Open(request);
}

// 0x0037b8e8
void MetSaveRemixScreen::EnterAndShow() {
    mUnknowndc = 0;
    mUnknowne0 = 0;
    mUnknown10->SetActivePanel(this);
    mUnknown10->OnUnknown00390088();
    mUnknowne8->SetSelected(kFirstButtonIndex);

    const HxStr playerFormat(ConfigText(kDialogueConfigCode, kPlayerKey));
    const HxStr player(FormatString(kPlayerFormat, TextOrEmpty(playerFormat), mUnknownc8));
    FindText(kPlayerPanelObject)->SetText(player);

    if (mPersona == nullptr) {
        mPersona = MetFrontEndState::shared()->GetFirstPersona();
    }
    mFreqNameText->SetText(
        HxStr(FormatString(kPersonaFormat, TextOrEmpty(mPersona->mUnknown140.mUnknown00))));

    const HxStr command(ConfigText(kDialogueConfigCode, kCommandKey));
    const HxStr instructions(
        FormatString(kCommandFormat, TextOrEmpty(command), TextOrEmpty(mUnknown94.mSlotName)));
    mInstructionsText->SetText(instructions);

    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    HxStr name;
    if (mUnknown104 != kEmptyText) {
        mRemixNameText->SetText(mUnknown104);
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

    MetHelpScreen::SetText(mUnknown38[0], mUnknown10->mUnknown68);
    MetHelpScreen::SelectPreset(HxStr(kHelpPreset));
    MetScreen::EnterAndShow();
}

// 0x0037c260
void MetSaveRemixScreen::OnUnknownSlot36() {
    if (mUnknown100 != 0) {
        mUnknown100 = 0;
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
        MetMsgScreen::SetOwnerPad(mUnknownc8);
        return;
    }

    if (mUnknown18 == kExitDiscard) {
        if (mUnknownfc != nullptr) {
            mUnknownfc->OnUnknownSlot2(kSaverBack);
        }
        return;
    }

    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    RecordPendingSave(mUnknown94,
                      mUnknownc8,
                      HxStr(TextOrEmpty(mRemixNameText->mPreWrapText)),
                      params.mLevelName,
                      mUnknownac,
                      QueryConfigValue(kAlbumNumberConfigCode));
}
