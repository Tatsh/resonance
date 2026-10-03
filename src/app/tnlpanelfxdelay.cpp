#include "app/tnlpanelfxdelay.h"

#include "app/apptunnel.h"

// NTSC-U/C: 0x004571f8, PAL: 0x00494728
void TnlPanelFXDelay::Fire([[maybe_unused]] float flFrame) {
    mTunnel->StartPanelFX(mRing, mSlice, mForward);
}
