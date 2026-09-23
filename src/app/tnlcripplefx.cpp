#include "app/tnlcripplefx.h"

#include "app/application.h"
#include "app/playsound.h"
#include "app/tnlplayer.h"
#include "game/forcefeedbackmgr.h"
#include "game/grooveworld.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/particlesys.h"
#include "rnd/transanim.h"
#include "rnd/view.h"

namespace {

// Path frames per song frame on the way out, before mRate applies.
constexpr float kLaunchRate = 6.0f;

// Path frames per song frame on the way back, before mRate applies.
constexpr float kReturnRate = -3.0f;

// Frames the path may run ahead of the song before the crippler turns back.
constexpr float kTurnBackLead = 6500.0f;

} // namespace

// 0x0043e1f0
TnlCrippleFX::TnlCrippleFX(int nIndex, float flRate)
    : mView(dynamic_cast<Rnd::View *>(
          Rnd::g_manager.Find(HxStr(FormatString("cripfx%d.view", nIndex))))),
      mPath(dynamic_cast<Rnd::TransAnim *>(
          Rnd::g_manager.Find(HxStr(FormatString("cripfx%d.path", nIndex))))),
      mParticleSys(dynamic_cast<Rnd::ParticleSys *>(
          Rnd::g_manager.Find(HxStr(FormatString("cripfx%d.ps", nIndex))))),
      mState(kStateIdle), mHitFrame(0.0f), mRate(flRate) {
    mView->SetShowing(0);
}

// 0x004568b8
void TnlCrippleFX::Start(const std::vector<TnlPlayer *> &targets, float flFrame) {
    mState = kStateRunning;
    mTargets = targets;
    mPath->SetRate(mRate * kLaunchRate);
    mPath->SetOffset(flFrame - flFrame * kLaunchRate * mRate);
    mView->SetShowing(1);
}

// 0x0043e500
void TnlCrippleFX::SetFrame(float flFrame) {
    mView->SetFrame(flFrame * mRate);
    if (mState == kStateIdle) {
        return;
    }
    mPath->SetFrame(flFrame);
    const float flPathFrame = mPath->mFilteredFrame;
    if (mState == kStateRunning) {
        if (flFrame + kTurnBackLead < flPathFrame) {
            mPath->SetRate(mRate * kReturnRate);
            mState = kStateReturning;
        }
    } else if (mState == kStateReturning) {
        if (flPathFrame < flFrame) {
            for (auto it = mTargets.begin(); it != mTargets.end(); ++it) {
                PlaySoundByName("SND_CRIPPLER_HIT");
                (*it)->SetCrippleFrame(flFrame);
                if (Application::shared()->GetWorld() != nullptr) {
                    Application::shared()->GetWorld()->mForceFeedback->PlayCrippleEffect(
                        (*it)->mPlayer);
                }
            }
            mHitFrame = flFrame;
            mState = kStateHit;
        }
    } else if (mState == kStateHit) {
        mState = kStateIdle;
        mView->SetShowing(0);
        mParticleSys->FreeAllParticles();
    }
}
