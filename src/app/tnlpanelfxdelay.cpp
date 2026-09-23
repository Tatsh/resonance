#include "app/tnlpanelfxdelay.h"

#include "app/apptunnel.h"

// 0x004571f8
void TnlPanelFXDelay::Fire() {
    mTunnel->StartPanelFX(mRing, mSlice, mForward);
}
