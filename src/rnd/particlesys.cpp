#include "rnd/particlesys.h"

#include <vector>

#include "os/failsink.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/particle.h"

namespace Rnd {

namespace {

// Value mLastFrame starts at, which makes the first SetFrameSelf() record the frame without
// emitting anything. Written as nine nines in the source rather than computed, because the bit
// pattern 0xcb18967f round-trips to exactly this and not to -1.0e7.
constexpr float kUnsetFrame = -9999999.0f;

} // namespace

// 0x0071aef0
HxStr g_particleSysClassName("ParticleSys");

// 0x0071aef8
ParticleSys *(*g_pfnNewParticleSys)(const HxStr &name) = NewParticleSys;

// 0x0052c6a8
// The printer has no case for kModeSprite, so a sprite system writes no mode at all. The gap is in
// the shipped build.
FailSink &PrintParticleMode(FailSink &sink, ParticleSys::Mode nMode) {
    if (nMode == ParticleSys::kModePoint) {
        sink.Print("Point");
    } else if (nMode == ParticleSys::kModeLine) {
        sink.Print("Line");
    }
    return sink;
}

// 0x0052b4a8
const HxStr &ParticleSys::ClassName() const {
    return g_particleSysClassName;
}

// 0x0052c490
void ParticleSys::StartAnim() {
    FreeAllParticles();
    Animatable::StartAnim();
}

// 0x0052c4c0
void ParticleSys::SetFrameSelf(float flFrame) {
    if (mLastFrame != kUnsetFrame) {
        const float flDeltaFrames = flFrame - mLastFrame;
        UpdateParticles(flDeltaFrames);
        SpawnParticles(flDeltaFrames);
    }
    mLastFrame = flFrame;
}

// 0x0052c318
void ParticleSys::RemoveObjectRefs() {
    if (mMat != nullptr) {
        mMat->RemoveRef(this);
    }
    if (mParticlesOwner != nullptr) {
        mParticlesOwner->RemoveRef(this);
    }
    mParticlesOwner->mSharers.remove(this);
}

// 0x0052b768
ParticleSys *NewParticleSys(const HxStr &name) {
    // The binary bills the allocation to the tag "Rnd::ParticleSys" and the object is 0x220 bytes.
    return new ParticleSys(name);
}

} // namespace Rnd
