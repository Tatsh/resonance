#include "met/metpausebasescreen.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "game/gameparams.h"
#include "game/grooveworld.h"
#include "met/metfrontendstate.h"
#include "met/metmsgscreen.h"
#include "met/metstrings.h"
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

// A command code past MetScreenCommandCode's range that a pause screen treats as a resume.
constexpr int kCommandResume = 10;

// The MetFrontEndState phase in which a select may restart, and the phase a quit records.
constexpr int kTutorialPhase = 5;
constexpr int kQuitPhase = 4;

inline void QueueUnpause() {
    UnpauseGameSystemMsg msg;
    Application::shared()->GetGameManager()->QueueMessage(&msg);
}

inline void ShowConfirmation(const char *pszDialogue,
                             const HxStr &title,
                             const HxStr &text,
                             MetScreen *pOwner) {
    std::vector<HxStr> buttons;
    buttons.push_back(MetText(kMetStrMsgNO, kNoButton));
    buttons.push_back(MetText(kMetStrMsgYES, kYesButton));
    MetMsgScreen::Show(HxStr(pszDialogue), title, text, kConfirmButtonCount, buttons, pOwner);
}

} // namespace

MetPauseBaseScreen::MetPauseBaseScreen(MetRenderer *pRenderer,
                                       int nPriority,
                                       const HxStr &name,
                                       const HxStr &directory,
                                       const HxStr &file)
    : MetScreenMultiSoundBank(pRenderer, nPriority, name, directory, file), mExitAction(kExitNone),
      mReturnPanel(kNoText) {
}

MetPauseBaseScreen::~MetPauseBaseScreen() {
}

void MetPauseBaseScreen::HandleCommand(const MetScreenCommand *pCommand) {
    switch (pCommand->mCommand) {
    case kCommandResume:
        PlayPauseSound(pCommand->mPadIndex);
        mExitAction = kExitResume;
        break;
    case kMetScreenCommandBack:
        PlayPauseSound(pCommand->mPadIndex);
        mExitAction = kExitQuit;
        break;
    case kMetScreenCommandSelect: {
        bool bAllowed = false;
        if ((Application::shared()->GetPlayMode() == kPlayModeGame &&
             Application::shared()->GetGameMode() != kGameModeNet) ||
            MetFrontEndState::shared()->mPendingTransition == kTutorialPhase) {
            bAllowed = true;
        }
        if (!bAllowed) {
            return;
        }
        PlayPauseSound(pCommand->mPadIndex);
        mExitAction = kExitRestart;
        break;
    }
    default:
        return;
    }
    ActivateNamedPanel(HxStr(kNoText));
    BeginExit();
}

void MetPauseBaseScreen::EnterAndShow() {
    // Yes, the binary copies the settings and never reads the copy.
    GameParams params(*Application::shared()->GetGameManager()->GetParams());
    for (std::vector<Rnd::Text *>::size_type i = 0; i < mOptionTexts.size(); ++i) {
        mOptionTexts[i]->SetText(mOptionLabels[i]);
    }
    MetScreen::EnterAndShow();
}

void MetPauseBaseScreen::OnExitFinished() {
    if (mExitAction == kExitQuit) {
        ShowConfirmation(kQuitDialogue,
                         MetText(kMetStrPauseQuit, kQuitTitle),
                         MetText(kMetStrPauseConfirm, kQuitText),
                         this);
    } else if (mExitAction == kExitRestart) {
        ShowConfirmation(kRestartDialogue,
                         MetText(kMetStrPauseRestart, kRestartTitle),
                         MetText(kMetStrRestartConfirm, kRestartText),
                         this);
    } else {
        QueueUnpause();
    }
}

void MetPauseBaseScreen::OnMsgScreenDismissed(const HxStr &name, int nChoice) {
    if (name == kQuitDialogue) {
        if (nChoice == kChoiceNo) {
            ActivateNamedPanel(mReturnPanel);
            PushNamedScreen(mReturnPanel);
        } else if (nChoice == kChoiceYes) {
            QueueUnpause();
            MetFrontEndState *pState = MetFrontEndState::shared();
            pState->mLastTransition = pState->mPendingTransition;
            pState->mPendingTransition = kQuitPhase;
            Application::shared()->GetGameManager()->GetWorld()->PostQuit();
        }
    } else if (name == kRestartDialogue) {
        if (nChoice == kChoiceNo) {
            ActivateNamedPanel(mReturnPanel);
            PushNamedScreen(mReturnPanel);
        } else if (nChoice == kChoiceYes) {
            QueueUnpause();
            Application::shared()->GetGameManager()->GetWorld()->PostRestart();
        }
    }
}

void MetPauseBaseScreen::PlaySlideSound(int) {
}

void MetPauseBaseScreen::PlayLeaveSound(int) {
}

void MetPauseBaseScreen::PlayHighSound(int) {
}

void MetPauseBaseScreen::PlayCycleLeftSound(int) {
}

void MetPauseBaseScreen::PlayCycleRightSound(int) {
}

void MetPauseBaseScreen::PlayPauseSound(int nSelector) {
    MetScreenMultiSoundBank::PlaySlideSound(nSelector);
}

MetPauseBaseScreen *MetPauseBaseScreen::New(MetRenderer *pRenderer,
                                            int nPriority,
                                            const HxStr &name,
                                            const HxStr &directory,
                                            const HxStr &file) {
    return new MetPauseBaseScreen(pRenderer, nPriority, name, directory, file);
}
