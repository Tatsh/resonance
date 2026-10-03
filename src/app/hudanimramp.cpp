#include "app/hudanimramp.h"

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"

// NTSC-U/C: 0x0042a888, PAL: 0x00465bd8
HudAnimRamp::HudAnimRamp(const HxStr &name, float flFrom, float flTo, float flDuration) {
    mAnim = dynamic_cast<Rnd::Animatable *>(Rnd::g_manager.Find(name));
    mAnim->SetFrame(0.0f);
    mRamp.SetRange(flFrom, flTo, flDuration);
}

// NTSC-U/C: 0x0042a958, PAL: 0x00465ca8
void HudAnimRamp::SetTarget(float flTarget) {
    mRamp.SetTarget(flTarget);
}

// NTSC-U/C: 0x0042a978, PAL: 0x00465cc8
void HudAnimRamp::Jump(float flTarget) {
    mRamp.Jump(flTarget);
}

// NTSC-U/C: 0x0042a998, PAL: 0x00465ce8
void HudAnimRamp::Update(float flTime) {
    if (mRamp.Update(flTime) != 0) {
        mAnim->SetFrame(mRamp.Value());
    }
}
