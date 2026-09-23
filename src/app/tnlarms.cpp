#include "app/tnlarms.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/particlesys.h"
#include "rnd/view.h"

namespace {

// Trigger frame of an idle burst, far beyond a song's length.
constexpr float kNoFrame = 1e9f;

// Local player views "tnl local1.view" through "tnl local4.view".
constexpr int kFirstLocalView = 1;
constexpr int kLastLocalView = 4;

constexpr int kEmitterCount = 3;

// Frames after the trigger during which the systems emit, and after which the view hides.
constexpr float kEmitFrames = 1920.0f;
constexpr float kShowFrames = 9600.0f;

} // namespace

TnlArms::TnlArms() : mStartFrame(kNoFrame) {
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("arms.view")));
    mView->SetShowing(0);
    for (int i = kFirstLocalView; i <= kLastLocalView; ++i) {
        Rnd::View *pLocal = dynamic_cast<Rnd::View *>(
            Rnd::g_manager.Find(HxStr(FormatString("tnl local%d.view", i))));
        pLocal->RemoveDraw(mView);
    }

    mEmitters.resize(kEmitterCount, TnlEmitter());
    for (unsigned i = 0; i < mEmitters.size(); ++i) {
        mEmitters[i].Attach(dynamic_cast<Rnd::ParticleSys *>(
            Rnd::g_manager.Find(HxStr(FormatString("arms%d.ps", i)))));
    }
}

void TnlArms::Start(float flFrame) {
    mStartFrame = flFrame;
    for (unsigned i = 0; i < mEmitters.size(); ++i) {
        mEmitters[i].Restart();
    }
    mView->SetShowing(1);
}

void TnlArms::SetFrame(float flFrame) {
    const float flElapsed = flFrame - mStartFrame;
    if (flElapsed < 0.0f) {
        return;
    }
    if (flElapsed < kEmitFrames) {
        mView->SetFrame(flFrame);
    } else if (flElapsed < kShowFrames) {
        mView->SetFrame(flFrame);
        for (unsigned i = 0; i < mEmitters.size(); ++i) {
            mEmitters[i].Stop();
        }
    } else {
        mStartFrame = kNoFrame;
        mView->SetShowing(0);
    }
}
