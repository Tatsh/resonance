#include "app/tnlpanelfxdelay.h"

#include "app/apptunnel.h"

void TnlPanelFXDelay::Fire([[maybe_unused]] float flFrame) {
    mTunnel->StartPanelFX(mRing, mSlice, mForward);
}
