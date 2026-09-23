#include "app/tnlboundary.h"

#include <cstring>

#include "app/application.h"
#include "app/tunnelcache.h"
#include "game/gamemanagerimpl.h"
#include "game/playmap.h"
#include "math/transform.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

constexpr int kFramesPerBar = 1920;

// The view's animation runs this many frames ahead of the distance to the boundary.
constexpr float kAnimLead = 2000.0f;

// Distance past a boundary, a quarter bar, at which the marker moves on to the next step.
constexpr float kPassedFrames = 480.0f;

// Set the local transform of pView to the tunnel path transform at the start of nBar.
inline void PlaceOnPath(Rnd::View *pView, int nBar) {
    Transform xfm;
    GetCachedTunnelObject()->GetPathXfm(&xfm, static_cast<float>(nBar * kFramesPerBar));
    std::memcpy(pView->mLocalXfm, &xfm, sizeof(pView->mLocalXfm));
    pView->mDirty = 1;
    pView->UpdateWorldXfm(nullptr, 0);
}

} // namespace

TnlBoundary::TnlBoundary(PlayMap *pPlayMap)
    : mPlayMap(pPlayMap), mView(nullptr), mStep(0), mStepCount(pPlayMap->Slot10()) {
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("boundary.view")));
    mText = dynamic_cast<Rnd::Text *>(Rnd::g_manager.Find(HxStr("boundary msg")));
    PlaceOnPath(mView, mStep);
    mText->SetShowing(Application::shared()->GetPlayMode() == kPlayModeGame ? 1 : 0);
    UpdateText();
}

void TnlBoundary::SetFrame(float flFrame) {
    const float flPast = flFrame - static_cast<float>(mStep * kFramesPerBar);
    mView->SetFrame(flPast + kAnimLead);
    if (kPassedFrames < flPast) {
        mStep = mPlayMap->FollowingStepBar(mStep);
        PlaceOnPath(mView, mStep);
        UpdateText();
    }
}

void TnlBoundary::UpdateText() {
    HxStr message("");
    if (Application::shared()->GetPlayMode() == kPlayModeGame) {
        const int nSection = mPlayMap->Slot13(mStep);
        if (nSection == 0) {
            message = "START";
        } else if (nSection == mStepCount - 1) {
            message = "FINAL\nSECTION";
        } else if (nSection == mStepCount) {
            message = "FINISH";
        }
    }
    mText->SetText(message);
}
