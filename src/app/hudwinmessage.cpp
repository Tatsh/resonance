#include "app/hudwinmessage.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "gfx/gfxdevice.h"
#include "os/hxstr.h"
#include "rnd/text.h"

namespace {

// Start time the constructor records. No step starts at it.
constexpr float kNoStart = 1.0e9f;

// Blend factor and texture inset of the frame feedback the sequence draws over.
constexpr float kFeedbackAlpha = 0.9f;
constexpr int kFeedbackInset = 100;

// Difficulties GameManagerImpl::GetDifficulty() reports, the settings field GameParams::Print()
// labels `difficulty=`. Each selects one congratulation.
constexpr int kDifficultyTrue = 0;
constexpr int kDifficultySuper = 1;
constexpr int kDifficultyMega = 2;

// Font scale of each message, relative to the message's size at construction.
constexpr float kCongratsScale = 2.5f;
constexpr float kTeamScale = 2.0f;
constexpr float kThanksScale = 1.7f;

// Time each message stays between its fades, and the time between the steps.
constexpr float kMessageHold = 8000.0f;
constexpr float kCreditDelay = 10000.0f;
constexpr float kPromptDelay = 8000.0f;

} // namespace

HudWinMessage::HudWinMessage()
    : mStart(kNoStart), mState(kStateIdle), mMessage(HxStr("HUD winmsg")),
      mPrompt(HxStr("HUD genmsg.txt")) {
}

void HudWinMessage::SetFrame(float flTime) {
    if (mState == kStateIdle) {
        return;
    }

    mMessage.SetFrame(flTime);
    const float flElapsed = flTime - mStart;

    if (mState == kStateStart) {
        mStart = flTime;
        g_gfxDevice.mFeedbackAlpha = kFeedbackAlpha;
        g_gfxDevice.mFeedbackInset = kFeedbackInset;
        g_gfxDevice.mFeedbackRect = GfxDevice::Rect{0.0f, 0.0f, 1.0f, 1.0f};

        const char *pszCongrats;
        switch (Application::shared()->GetGameManager()->GetDifficulty()) {
        case kDifficultyTrue:
            pszCongrats = "CONGRATULATIONS!\n\nYOU ARE NOW A\nTRUE FREQ";
            break;
        case kDifficultySuper:
            pszCongrats = "CONGRATULATIONS!\n\nYOU ARE NOW A\nSUPER FREQ!";
            break;
        case kDifficultyMega:
            pszCongrats = "CONGRATULATIONS!\n\nYOU ARE NOW A\nMEGA FREQ!!";
            break;
        default:
            pszCongrats = "Yup";
            break;
        }
        mMessage.Show(HxStr(pszCongrats), kCongratsScale, kMessageHold);
        mState = kStateCongrats;
    } else if (mState == kStateCongrats) {
        if (kCreditDelay < flElapsed) {
            mStart = flTime;
            mMessage.Show(HxStr("TEAM FREQUENCY<\nBELIEVES THERE IS\nNO GREATER HIGH\nTHAN MAKING "
                                "MUSIC.\n"),
                          kTeamScale,
                          kMessageHold);
            mState = kStateTeam;
        }
    } else if (mState == kStateTeam) {
        if (kCreditDelay < flElapsed) {
            mStart = flTime;
            mMessage.Show(HxStr("WE HOPE YOU ENJOYED\nSHAKING YOUR BOOTY,\nSHOWING OFF YOUR "
                                "SKILLZ,\nAND CRANKING UP YOUR\nFAVORITE FREQUENCY< MIX!"),
                          kThanksScale,
                          kMessageHold);
            mState = kStateThanks;
        }
    } else if (mState == kStateThanks) {
        if (kPromptDelay < flElapsed) {
            mPrompt.Show(HxStr("Press the START button to exit"));
            mState = kStatePrompt;
        }
    }
}

void HudWinMessage::HidePrompt() {
    mPrompt.Hide();
}

void HudWinMessage::Draw() {
    if (mState == kStateIdle) {
        return;
    }
    g_gfxDevice.SetupGsDrawContext();
    mMessage.mText->DrawSelf(); // Yes, the binary discards this result.
    mPrompt.mText->Draw();
}
