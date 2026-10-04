#include "app/tnlboundary.h"

#include "app/application.h"
#include "app/tnlutil.h"
#include "game/gamemanagerimpl.h"
#include "game/playmap.h"
#include "met/metstrings.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

constexpr int kFramesPerBar = 1920;

// The view's animation runs this many frames ahead of the distance to the boundary.
constexpr float kAnimLead = 2000.0f;

// Distance past a boundary, a quarter bar, at which the marker moves on to the next step.
constexpr float kPassedFrames = 480.0f;

} // namespace

TnlBoundary::TnlBoundary(PlayMap *pPlayMap)
    : mPlayMap(pPlayMap), mView(nullptr), mStep(0), mStepCount(pPlayMap->GetNumSections()) {
    mView = dynamic_cast<Rnd::View *>(Rnd::TheManager.Find(HxStr("boundary.view")));
    mText = dynamic_cast<Rnd::Text *>(Rnd::TheManager.Find(HxStr("boundary msg")));
    PlaceOnPath(*mView, static_cast<float>(mStep * kFramesPerBar));
    mText->SetShowing(Application::shared()->GetPlayMode() == kPlayModeGame ? 1 : 0);
    UpdateText();
}

void TnlBoundary::SetFrame(float flFrame) {
    const float flPast = flFrame - static_cast<float>(mStep * kFramesPerBar);
    mView->SetFrame(flPast + kAnimLead);
    if (kPassedFrames < flPast) {
        mStep = mPlayMap->FollowingStepBar(mStep);
        PlaceOnPath(*mView, static_cast<float>(mStep * kFramesPerBar));
        UpdateText();
    }
}

void TnlBoundary::UpdateText() {
    HxStr message("");
    if (Application::shared()->GetPlayMode() == kPlayModeGame) {
        const int nSection = mPlayMap->GetAbsoluteSectionIndex(mStep);
        if (nSection == 0) {
            message = MetText(kMetStrIngStart, "START");
        } else if (nSection == mStepCount - 1) {
            message = MetText(kMetStrIngFinal, "FINAL\nSECTION");
        } else if (nSection == mStepCount) {
            message = MetText(kMetStrIngFinish, "FINISH");
        }
    }
    mText->SetText(message);
}
