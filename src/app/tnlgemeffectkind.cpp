#include "app/tnlgemeffectkind.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/particlesys.h"

namespace {

// Drawing cost of one particle gem, against TnlGemManager's budget.
constexpr float kParticleCost = 2.0f;

} // namespace

// 0x004121d0
TnlGemEffectKind::TnlGemEffectKind(const char *pszName) {
    mParticleSys = dynamic_cast<Rnd::ParticleSys *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s.ps", pszName))));
    mParticleSys->FreeAllParticles();
    mCost = kParticleCost;
}
