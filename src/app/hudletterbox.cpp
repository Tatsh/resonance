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

// 0x0041b3b0
HudLetterbox::HudLetterbox() {
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("HUD letterbox.view")));
    mView->SetShowing(0);
    mRamp.SetRange(kRestFrame, kFullFrame, kRampDuration);
}

// 0x0042a608
void HudLetterbox::SetFrame(float flTime) {
    mRamp.Update(flTime); // Yes, the binary discards this result.
    mView->SetFrame(mRamp.Value());
    mView->SetShowing(mRamp.Value() != kRestFrame);
}

// 0x0042a698
void HudLetterbox::SetTarget(float flTarget) {
    mRamp.SetTarget(flTarget);
}

// 0x0042a6b8
void HudLetterbox::Jump(float flTarget) {
    mRamp.Jump(flTarget);
}
