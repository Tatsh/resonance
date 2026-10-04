#include "app/hudanalogstick.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"

HudAnalogStick::HudAnalogStick() {
    mMesh = dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(HxStr("HUD1 analog_stick.mesh")));
    mInOutMat = dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr("HUD in_out_stick.mat")));
    mUpDownMat = dynamic_cast<Rnd::Mat *>(Rnd::TheManager.Find(HxStr("HUD up_down_stick.mat")));
    SetShowing(0);
}

void HudAnalogStick::SetMotion(const HxStr &motion) {
    if (motion == "in_out") {
        mMesh->SetMat(mInOutMat);
    } else if (motion == "up_down") {
        mMesh->SetMat(mUpDownMat);
    }
}

void HudAnalogStick::SetShowing(int nShowing) {
    mMesh->SetShowing(nShowing);
}
