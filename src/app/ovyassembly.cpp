#include "app/ovyassembly.h"

#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"

// NTSC-U/C: 0x0042a888, PAL: 0x00465bd8
OvyAssembly::OvyAssembly(const HxStr &name, float flFrom, float flTo, float flDuration) {
    mAnim = dynamic_cast<Rnd::Animatable *>(Rnd::TheManager.Find(name));
    mAnim->SetFrame(0.0f);
    mRamp.SetParams(flFrom, flTo, flDuration);
}

// NTSC-U/C: 0x0042a958, PAL: 0x00465ca8
void OvyAssembly::SetTarget(float flTarget) {
    mRamp.SetTarget(flTarget);
}

// NTSC-U/C: 0x0042a978, PAL: 0x00465cc8
void OvyAssembly::Jump(float flTarget) {
    mRamp.Jump(flTarget);
}

// NTSC-U/C: 0x0042a998, PAL: 0x00465ce8
void OvyAssembly::Execute(float flTime) {
    if (mRamp.Execute(flTime) != 0) {
        mAnim->SetFrame(mRamp.Val());
    }
}
