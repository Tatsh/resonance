#include "app/hudanimramp.h"

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"

// 0x0042a888
HudAnimRamp::HudAnimRamp(const HxStr &name, float flFrom, float flTo, float flDuration) {
    mAnim = dynamic_cast<Rnd::Animatable *>(Rnd::g_manager.Find(name));
    mAnim->SetFrame(0.0f);
    mRamp.SetRange(flFrom, flTo, flDuration);
}

// 0x0042a958
void HudAnimRamp::SetTarget(float flTarget) {
    mRamp.SetTarget(flTarget);
}

// 0x0042a978
void HudAnimRamp::Jump(float flTarget) {
    mRamp.Jump(flTarget);
}

// 0x0042a998
void HudAnimRamp::Update(float flTime) {
    if (mRamp.Update(flTime) != 0) {
        mAnim->SetFrame(mRamp.Value());
    }
}
