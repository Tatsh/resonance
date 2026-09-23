#include "app/hudletterbox.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/view.h"

namespace {

// Frames the view's animation runs between, and the time the ramp takes to cover them.
constexpr float kRestFrame = 0.0f;
constexpr float kFullFrame = 100.0f;
constexpr float kRampDuration = 480.0f;

} // namespace

HudLetterbox::HudLetterbox() {
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("HUD letterbox.view")));
    mView->SetShowing(0);
    mRamp.SetRange(kRestFrame, kFullFrame, kRampDuration);
}

void HudLetterbox::SetFrame(float flTime) {
    mRamp.Update(flTime); // Yes, the binary discards this result.
    mView->SetFrame(mRamp.Value());
    mView->SetShowing(mRamp.Value() != kRestFrame);
}

void HudLetterbox::SetTarget(float flTarget) {
    mRamp.SetTarget(flTarget);
}

void HudLetterbox::Jump(float flTarget) {
    mRamp.Jump(flTarget);
}
