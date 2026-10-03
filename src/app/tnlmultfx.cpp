#include "app/tnlmultfx.h"

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/particlesys.h"
#include "rnd/transanim.h"

// NTSC-U/C: 0x0043d008, PAL: 0x00479708
TnlMultFX::TnlMultFX() : mActive(0) {
    mRange.SetAnim(dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(HxStr("multfx.path"))));
    mEmitter.Attach(dynamic_cast<Rnd::ParticleSys *>(Rnd::g_manager.Find(HxStr("multfx.ps"))));
    mAltEmitter.Attach(dynamic_cast<Rnd::ParticleSys *>(Rnd::g_manager.Find(HxStr("multfxa.ps"))));
}

// NTSC-U/C: 0x004564a0, PAL: 0x004939d0
void TnlMultFX::Start(float flFrom, float flTo) {
    mRange.Play(flFrom, flTo);
    mEmitter.Restart();
    mAltEmitter.Restart();
    mActive = 1;
}

// NTSC-U/C: 0x00456528, PAL: 0x00493a58
void TnlMultFX::SetFrame(float flFrame) {
    mEmitter.GetParticleSys()->SetFrame(flFrame);
    if (mRange.Update(flFrame) == 0 && mActive != 0) {
        mEmitter.Stop();
        mAltEmitter.Stop();
        mActive = 0;
    }
}
