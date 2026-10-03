#include "app/tnlgemeffectkind.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/particlesys.h"

namespace {

// Drawing cost of one particle gem, against TnlGemManager's budget.
constexpr float kParticleCost = 2.0f;

} // namespace

// NTSC-U/C: 0x004121d0, PAL: 0x0044bcb0
TnlGemEffectKind::TnlGemEffectKind(const char *pszName) {
    mParticleSys = dynamic_cast<Rnd::ParticleSys *>(
        Rnd::TheManager.Find(HxStr(Rnd::MakeString("%s.ps", pszName))));
    mParticleSys->FreeAllParticles();
    mCost = kParticleCost;
}
