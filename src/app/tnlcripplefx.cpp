#include "app/tnlcripplefx.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/particlesys.h"
#include "rnd/transanim.h"
#include "rnd/view.h"

namespace {

// Path frames per song frame on the way out, before mRate applies.
constexpr float kLaunchRate = 6.0f;

} // namespace

TnlCrippleFX::TnlCrippleFX(int nIndex, float flRate)
    : mState(kStateIdle), mHitFrame(0.0f), mRate(flRate) {
    mView = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("cripfx%d.view", nIndex))));
    mPath = dynamic_cast<Rnd::TransAnim *>(
        Rnd::g_manager.Find(HxStr(FormatString("cripfx%d.path", nIndex))));
    mParticleSys = dynamic_cast<Rnd::ParticleSys *>(
        Rnd::g_manager.Find(HxStr(FormatString("cripfx%d.ps", nIndex))));
    mView->SetShowing(0);
}

void TnlCrippleFX::Start(const std::vector<TnlPlayer *> &targets, float flFrame) {
    mState = kStateRunning;
    mTargets = targets;
    mPath->SetRate(mRate * kLaunchRate);
    mPath->SetOffset(flFrame - flFrame * kLaunchRate * mRate);
    mView->SetShowing(1);
}
