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

// NTSC-U/C: 0x0041b3b0, PAL: 0x00455d08
HudLetterbox::HudLetterbox() {
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("HUD letterbox.view")));
    mView->SetShowing(0);
    mRamp.SetRange(kRestFrame, kFullFrame, kRampDuration);
}

// NTSC-U/C: 0x0042a608, PAL: 0x00465958
void HudLetterbox::SetFrame(float flTime) {
    mRamp.Update(flTime); // Yes, the binary discards this result.
    mView->SetFrame(mRamp.Value());
    mView->SetShowing(mRamp.Value() != kRestFrame);
}

// NTSC-U/C: 0x0042a698, PAL: 0x004659e8
void HudLetterbox::SetTarget(float flTarget) {
    mRamp.SetTarget(flTarget);
}

// NTSC-U/C: 0x0042a6b8, PAL: 0x00465a08
void HudLetterbox::Jump(float flTarget) {
    mRamp.Jump(flTarget);
}
