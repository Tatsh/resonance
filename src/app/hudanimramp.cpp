#include "app/hudanimramp.h"

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"

HudAnimRamp::HudAnimRamp(const HxStr &name, float flFrom, float flTo, float flDuration) {
    mAnim = dynamic_cast<Rnd::Animatable *>(Rnd::g_manager.Find(name));
    mAnim->SetFrame(0.0f);
    mRamp.SetRange(flFrom, flTo, flDuration);
}

void HudAnimRamp::SetTarget(float flTarget) {
    mRamp.SetTarget(flTarget);
}

void HudAnimRamp::Jump(float flTarget) {
    mRamp.Jump(flTarget);
}

void HudAnimRamp::Update(float flTime) {
    if (mRamp.Update(flTime) != 0) {
        mAnim->SetFrame(mRamp.Value());
    }
}
