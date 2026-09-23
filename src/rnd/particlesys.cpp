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

// Particles the constructor sizes the pool to.
constexpr int kDefaultPoolSize = 10;

// Defaults the constructor writes into the parameter block.
constexpr float kDefaultBubblePeriod = 10.0f;
constexpr float kDefaultBubbleSize = 1.0f;
constexpr float kDefaultLife = 100.0f;
constexpr float kDefaultSpeed = 1.0f;
constexpr float kDefaultEmitRate = 10.0f;
constexpr float kDefaultSize = 1.0f;
constexpr Color kDefaultColor = {1.0f, 1.0f, 1.0f, 1.0f};
constexpr Vector3 kOrigin = {0.0f, 0.0f, 0.0f, 1.0f};
constexpr Plane kDefaultCollidePlane = {0.0f, 0.0f, 1.0f, 0.0f};
constexpr int kDefaultLineLength = 1;

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

// 0x005254a0
// The binary sizes the pool from a temporary particle whose four vector padding words are 1.0.
ParticleSys::ParticleSys(const HxStr &name)
    : Object(name), mParticlesOwner(this), mParticles(kDefaultPoolSize), mLastFrame(kUnsetFrame),
      mCollide(0), mMat(nullptr), mMode(kModeLine), mBubble(0), mReadZ(1),
      mLineLength(kDefaultLineLength) {
    mBubblePeriod.x = kDefaultBubblePeriod;
    mBubblePeriod.y = kDefaultBubblePeriod;
    mBubbleSize.x = kDefaultBubbleSize;
    mBubbleSize.y = kDefaultBubbleSize;
    mLife.x = kDefaultLife;
    mLife.y = kDefaultLife;
    mPosLow = kOrigin;
    mPosHigh = kOrigin;
    mSpeed.x = kDefaultSpeed;
    mSpeed.y = kDefaultSpeed;
    mPitch.x = 0.0f;
    mPitch.y = 0.0f;
    mYaw.x = 0.0f;
    mYaw.y = 0.0f;
    mEmitRateLow = kDefaultEmitRate;
    mEmitRateHigh = kDefaultEmitRate;
    mSizeLow = kDefaultSize;
    mSizeHigh = kDefaultSize;
    mStartColorLow = kDefaultColor;
    mStartColorHigh = kDefaultColor;
    mEndColorLow = kDefaultColor;
    mEndColorHigh = kDefaultColor;
    mCollidePlane = kDefaultCollidePlane;
    mForce = kOrigin;
    AddObjectRefs();
}

// 0x00524f58
ParticleSys::~ParticleSys() {
    RemoveObjectRefs();
    ReleaseAllRefs();
}

// 0x00521d38
void ParticleSys::Copy(const Object *pSource, unsigned nFlags) {
    const ParticleSys *pSys = dynamic_cast<const ParticleSys *>(pSource);

    Animatable::Copy(pSource, nFlags);
    Transformable::Copy(pSource, nFlags);
    Drawable::Copy(pSource, nFlags);
    RemoveObjectRefs();

    mLife = pSys->mLife;
    mPosLow = pSys->mPosLow;
    mPosHigh = pSys->mPosHigh;
    mSpeed = pSys->mSpeed;
    mPitch = pSys->mPitch;
    mYaw = pSys->mYaw;
    mEmitRateLow = pSys->mEmitRateLow;
    mEmitRateHigh = pSys->mEmitRateHigh;
    mSizeLow = pSys->mSizeLow;
    mSizeHigh = pSys->mSizeHigh;
    mStartColorLow = pSys->mStartColorLow;
    mStartColorHigh = pSys->mStartColorHigh;
    mEndColorLow = pSys->mEndColorLow;
    mEndColorHigh = pSys->mEndColorHigh;
    mCollide = pSys->mCollide;
    mCollidePlane = pSys->mCollidePlane;
    mForce = pSys->mForce;
    mMat = pSys->mMat;
    mMode = pSys->mMode;
    mBubblePeriod = pSys->mBubblePeriod;
    mBubbleSize = pSys->mBubbleSize;
    mBubble = pSys->mBubble;
    mReadZ = pSys->mReadZ;

    if ((nFlags & kCopyShareParticles) == 0 && pSys->mParticlesOwner == pSys) {
        mParticlesOwner = this;
        mParticles = pSys->mParticles;
    } else {
        mParticlesOwner = pSys->mParticlesOwner;
        if (mParticlesOwner != this) {
            mParticles.clear();
        }
    }
    AddObjectRefs();
}

// 0x00524318
void ParticleSys::Replace(Object *pFrom, Object *pTo) {
    Animatable::Replace(pFrom, pTo);
    Transformable::Replace(pFrom, pTo);
    Drawable::Replace(pFrom, pTo);

    if (mMat == pFrom && mMat != nullptr) {
        pFrom->RemoveRef(this);
        mMat = dynamic_cast<Mat *>(pTo);
        if (mMat != nullptr) {
            mMat->AddRef(this);
        }
    }

    // A null owner matches a null pFrom, and RemoveObjectRefs() then dereferences it.
    if (mParticlesOwner == pFrom) {
        RemoveObjectRefs();
        if (pTo != nullptr) {
            mParticlesOwner = dynamic_cast<ParticleSys *>(pTo);
        } else {
            // Losing the owner takes a copy of its pool rather than discarding the particles.
            mParticles = mParticlesOwner->mParticles;
            mParticlesOwner = this;
        }
        AddObjectRefs();
    }
}

// 0x005241a8
void ParticleSys::AddObjectRefs() {
    if (mMat != nullptr) {
        mMat->AddRef(this);
    }
    if (mParticlesOwner != nullptr) {
        mParticlesOwner->AddRef(this);
    }

    if (mParticlesOwner == this) {
        Particle *pEnd = mParticles.data() + mParticles.size();
        for (Particle *pParticle = mParticles.data(); pParticle != pEnd; ++pParticle) {
            pParticle->mNext = pParticle + 1;
        }
        mFreeParticles = mParticles.data();
        for (ParticleSys *pSharer : mSharers) {
            pSharer->mLiveParticles = nullptr;
        }
    } else {
        mParticlesOwner->mSharers.push_back(this);
    }
    mUnknown100 = 0;
    mLiveParticles = nullptr;
}

// 0x00524a70
void ParticleSys::FreeAllParticles() {
    Particle *pParticle = mLiveParticles;
    while (pParticle != nullptr) {
        pParticle = FreeParticle(pParticle);
    }
}

// 0x0052c378
Particle *ParticleSys::AllocParticle() {
    ParticleSys *pOwner = mParticlesOwner;
    Particle *pParticle = pOwner->mFreeParticles;
    if (pParticle == pOwner->mParticles.data() + pOwner->mParticles.size()) {
        return nullptr;
    }

    pOwner->mFreeParticles = pParticle->mNext;
    if (mLiveParticles != nullptr) {
        mLiveParticles->mPrev = pParticle;
    }
    pParticle->mNext = mLiveParticles;
    mLiveParticles = pParticle;
    pParticle->mPrev = pParticle;
    return pParticle;
}

// 0x0052c3c0
Particle *ParticleSys::FreeParticle(Particle *pParticle) {
    if (pParticle == nullptr) {
        return nullptr;
    }
    if (pParticle->mPrev == nullptr) {
        g_failSink.Print("Tried to refree particle from ")
            ->Format("\"%s\"", mName.mStr != nullptr ? mName.mStr : g_szEmptyString)
            ->Print("\n");
        return nullptr;
    }

    if (pParticle == mLiveParticles) {
        mLiveParticles = pParticle->mNext;
    } else {
        pParticle->mPrev->mNext = pParticle->mNext;
    }
    // Yes, a new head inherits the freed head's mPrev, which points at the freed particle rather
    // than at the new head.
    if (pParticle->mNext != nullptr) {
        pParticle->mNext->mPrev = pParticle->mPrev;
    }

    Particle *pNext = pParticle->mNext;
    pParticle->mNext = mParticlesOwner->mFreeParticles;
    mParticlesOwner->mFreeParticles = pParticle;
    pParticle->mPrev = nullptr;
    return pNext;
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
