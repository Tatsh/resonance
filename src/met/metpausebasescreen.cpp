#include "met/metpausebasescreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/grooveworld.h"
#include "met/metfrontendstate.h"
#include "met/metmsgscreen.h"
#include "msg/unpausegamesystemmsg.h"
#include "os/hxstr.h"
#include "rnd/text.h"

namespace {

static const char *const kNoText = "";
static const char *const kNoButton = "NO";
static const char *const kYesButton = "YES";

static const char *const kQuitDialogue = "quit_game_pause_screen_check";
static const char *const kQuitTitle = "Quit";
static const char *const kQuitText = "Are you sure that you want to quit?";
static const char *const kRestartDialogue = "restart_game_pause_screen_check";
static const char *const kRestartTitle = "Restart";
static const char *const kRestartText = "Are you sure that you want to restart?";

constexpr int kConfirmButtonCount = 2;

enum ConfirmChoice {
    kChoiceNo = 0,
    kChoiceYes = 1,
};

// A command code past MetScreenCommandCode's range, which a pause screen treats as a resume.
constexpr int kCommandResume = 10;

// The MetFrontEndState phase in which a select may restart, and the phase a quit records.
constexpr int kTutorialPhase = 5;
constexpr int kQuitPhase = 4;

inline void QueueUnpause() {
    UnpauseGameSystemMsg msg;
    Application::shared()->GetGameManager()->QueueMessage(&msg);
}

inline void ShowConfirmation(const char *pszDialogue,
                             const char *pszTitle,
                             const char *pszText,
                             MetScreen *pOwner) {
    std::vector<HxStr> buttons;
    buttons.push_back(HxStr(kNoButton));
    buttons.push_back(HxStr(kYesButton));
    MetMsgScreen::Show(
        HxStr(pszDialogue), HxStr(pszTitle), HxStr(pszText), kConfirmButtonCount, buttons, pOwner);
}

} // namespace

// 0x00317d40
MetPauseBaseScreen::MetPauseBaseScreen(MetRenderer *pRenderer,
                                       int nPriority,
                                       const HxStr &name,
                                       const HxStr &directory,
                                       const HxStr &file)
    : MetScreenMultiSoundBank(pRenderer, nPriority, name, directory, file), mUnknown8c(kExitNone),
      mUnknown9c(kNoText) {
}

// 0x00317e90
MetPauseBaseScreen::~MetPauseBaseScreen() {
}

// 0x00318010
void MetPauseBaseScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kCommandResume:
        PlayPauseSound(pCommand->mPadIndex);
        mUnknown8c = kExitResume;
        break;
    case kMetScreenCommandBack:
        PlayPauseSound(pCommand->mPadIndex);
        mUnknown8c = kExitQuit;
        break;
    case kMetScreenCommandSelect: {
        bool bAllowed = false;
        if ((Application::shared()->GetPlayMode() == kPlayModeGame &&
             Application::shared()->GetGameMode() != kGameModeNet) ||
            MetFrontEndState::shared()->mUnknown18 == kTutorialPhase) {
            bAllowed = true;
        }
        if (!bAllowed) {
            return;
        }
        PlayPauseSound(pCommand->mPadIndex);
        mUnknown8c = kExitRestart;
        break;
    }
    default:
        return;
    }
    ActivateNamedPanel(HxStr(kNoText));
    BeginExit();
}

// 0x00318278
void MetPauseBaseScreen::EnterAndShow() {
    // Yes, the binary copies the settings and never reads the copy.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    for (std::vector<Rnd::Text *>::size_type i = 0; i < mUnknowna4.size(); ++i) {
        mUnknowna4[i]->SetText(mUnknown90[i]);
    }
    MetScreen::EnterAndShow();
}

// 0x00318418
void MetPauseBaseScreen::OnUnknownSlot36() {
    if (mUnknown8c == kExitQuit) {
        ShowConfirmation(kQuitDialogue, kQuitTitle, kQuitText, this);
    } else if (mUnknown8c == kExitRestart) {
        ShowConfirmation(kRestartDialogue, kRestartTitle, kRestartText, this);
    } else {
        QueueUnpause();
    }
}

// 0x00318b80
void MetPauseBaseScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kQuitDialogue) {
        if (nChoice == kChoiceNo) {
            ActivateNamedPanel(mUnknown9c);
            PushNamedScreen(mUnknown9c);
        } else if (nChoice == kChoiceYes) {
            QueueUnpause();
            MetFrontEndState *pState = MetFrontEndState::shared();
            pState->mUnknown1c = pState->mUnknown18;
            pState->mUnknown18 = kQuitPhase;
            Application::shared()->GetGameManager()->GetWorld()->PostExitMode2();
        }
    } else if (name == kRestartDialogue) {
        if (nChoice == kChoiceNo) {
            ActivateNamedPanel(mUnknown9c);
            PushNamedScreen(mUnknown9c);
        } else if (nChoice == kChoiceYes) {
            QueueUnpause();
            Application::shared()->GetGameManager()->GetWorld()->PostExitMode3();
        }
    }
}

// 0x0031bff0
void MetPauseBaseScreen::PlaySlideSound(int) {
}

// 0x0031bff8
void MetPauseBaseScreen::PlayLeaveSound(int) {
}

// 0x0031c000
void MetPauseBaseScreen::PlayHighSound(int) {
}

// 0x0031c008
void MetPauseBaseScreen::PlayCycleLeftSound(int) {
}

// 0x0031c010
void MetPauseBaseScreen::PlayCycleRightSound(int) {
}

// 0x0031c018
void MetPauseBaseScreen::PlayPauseSound(int nSelector) {
    MetScreenMultiSoundBank::PlaySlideSound(nSelector);
}

// 0x0031c038
MetPauseBaseScreen *MetPauseBaseScreen::New(MetRenderer *pRenderer,
                                            int nPriority,
                                            const HxStr &name,
                                            const HxStr &directory,
                                            const HxStr &file) {
    return new MetPauseBaseScreen(pRenderer, nPriority, name, directory, file);
}
