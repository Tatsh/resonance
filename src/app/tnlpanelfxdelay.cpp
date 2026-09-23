#include "app/tnlpanelfxdelay.h"

#include "app/apptunnel.h"

// 0x004571f8
void TnlPanelFXDelay::Fire([[maybe_unused]] float flFrame) {
    mTunnel->StartPanelFX(mRing, mSlice, mForward);
}
