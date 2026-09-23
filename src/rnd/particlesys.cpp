#include "rnd/particlesys.h"

#include <math.h>
#include <vector>

#include "math/color.h"
#include "math/plane.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/failsink.h"
#include "os/hxstr.h"
#include "os/random.h"
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

// Scale that maps a 31-bit NextRandomValue() result onto the unit range, 2 to the power of -31.
constexpr float kRandomUnitScale = 1.0f / 2147483648.0f;

// A random value interpolated from the high end of a range toward the low end.
inline float RandomInRange(float flLow, float flHigh) {
    return static_cast<float>(NextRandomValue()) * kRandomUnitScale * (flLow - flHigh) + flHigh;
}

// A full turn in radians, the float whose bit pattern is 0x40c90fda.
constexpr float kTwoPi = 6.2831855f;

// Move a point from local into world space. The binary open-codes this on VU0, accumulating the
// three basis rows scaled by the components and then the translation row.
inline void TransformPointByWorldXfm(Vector3 &point,
                                     const float (&xfm)[kXfmRowCount][kXfmRowFloatCount]) {
    const float flX = point.x;
    const float flY = point.y;
    const float flZ = point.z;
    point.x = xfm[0][0] * flX + xfm[1][0] * flY + xfm[2][0] * flZ + xfm[3][0];
    point.y = xfm[0][1] * flX + xfm[1][1] * flY + xfm[2][1] * flZ + xfm[3][1];
    point.z = xfm[0][2] * flX + xfm[1][2] * flY + xfm[2][2] * flZ + xfm[3][2];
}

// Signed distance of a point from a plane. UpdateParticles() evaluates it twice.
inline float DistanceToPlane(const Plane &plane, const Vector3 &point) {
    return plane.a * point.x + plane.b * point.y + plane.c * point.z + plane.d;
}

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
    mEmitAccumulator = 0.0f;
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

// 0x00524b70
void ParticleSys::UpdateParticles(float flDeltaFrames) {
    if (flDeltaFrames == 0.0f) {
        return;
    }

    Vector3 forceStep;
    forceStep.w = 1.0f;
    Vec3Scale(&mForce.x, flDeltaFrames, &forceStep.x);

    Particle *pParticle = mLiveParticles;
    while (pParticle != nullptr) {
        if (pParticle->mDeathFrame <= mFilteredFrame || mFilteredFrame < pParticle->mBirthFrame) {
            pParticle = FreeParticle(pParticle);
            continue;
        }

        if (mMode == kModeLine) {
            // Yes, the history runs through the quadwords after mPrevPos, over mVel for a line
            // length above 1.
            Vector3 *pHistory = &pParticle->mPrevPos;
            for (int nIndex = mLineLength - 1; nIndex > 0; --nIndex) {
                pHistory[nIndex] = pHistory[nIndex - 1];
            }
        }
        pParticle->mPrevPos = pParticle->mPos;

        Vector3 step;
        step.w = 1.0f;
        Vec3Scale(&pParticle->mVel.x, flDeltaFrames, &step.x);
        AddVec3(&pParticle->mPos.x, &step.x, &pParticle->mPos.x);

        if (mBubble != 0) {
            const float flRate =
                cosf(mFilteredFrame * pParticle->mBubbleFrequency + pParticle->mBubblePhase) *
                pParticle->mBubbleFrequency;
            step.w = 1.0f;
            Vec3Scale(&pParticle->mBubbleSize.x, flRate * flDeltaFrames, &step.x);
            AddVec3(&pParticle->mPos.x, &step.x, &pParticle->mPos.x);
        }

        // A particle that crosses the collision plane from its positive side this step reflects
        // its velocity about the plane and returns to its previous position.
        if (mCollide != 0 && DistanceToPlane(mCollidePlane, pParticle->mPrevPos) > 0.0f &&
            !(DistanceToPlane(mCollidePlane, pParticle->mPos) > 0.0f)) {
            Vector3 doubledNormal;
            doubledNormal.w = 1.0f;
            Vec3Scale(&mCollidePlane.a, 2.0f, &doubledNormal.x);
            const Vector3 normal = doubledNormal;
            const float flNormalVelocity = mCollidePlane.a * pParticle->mVel.x +
                                           mCollidePlane.b * pParticle->mVel.y +
                                           mCollidePlane.c * pParticle->mVel.z;
            doubledNormal.w = 1.0f;
            Vec3Scale(&normal.x, flNormalVelocity, &doubledNormal.x);
            const Vector3 reflection = doubledNormal;
            Vector3 reflected;
            reflected.w = 1.0f;
            Vec3Sub(&pParticle->mVel.x, &reflection.x, &reflected.x);
            pParticle->mVel = reflected;
            pParticle->mPos = pParticle->mPrevPos;
        }

        AddVec3(&pParticle->mVel.x, &forceStep.x, &pParticle->mVel.x);

        // The binary scales the colour rate through the routine at 0x00453e08.
        Color colorStep;
        colorStep.r = pParticle->mColVel.r * flDeltaFrames;
        colorStep.g = pParticle->mColVel.g * flDeltaFrames;
        colorStep.b = pParticle->mColVel.b * flDeltaFrames;
        colorStep.a = pParticle->mColVel.a * flDeltaFrames;
        AddColor(pParticle->mCol, colorStep, pParticle->mCol);

        pParticle = pParticle->mNext;
    }
}

// 0x005244b0
void ParticleSys::SpawnParticles(float flDeltaFrames) {
    if (flDeltaFrames <= 0.0f) {
        return;
    }
    mEmitAccumulator += RandomInRange(mEmitRateLow, mEmitRateHigh) * flDeltaFrames;

    while (mEmitAccumulator >= 1.0f) {
        Particle *pParticle = AllocParticle();
        if (pParticle == nullptr) {
            mEmitAccumulator = 0.0f;
            return;
        }
        pParticle->mBirthFrame = mFilteredFrame;
        pParticle->mDeathFrame = pParticle->mBirthFrame + RandomInRange(mLife.x, mLife.y);

        const float flSpeed = RandomInRange(mSpeed.x, mSpeed.y);
        const float flPitch = RandomInRange(mPitch.x, mPitch.y);
        const float flYaw = RandomInRange(mYaw.x, mYaw.y);
        const float flCosPitch = cosf(flPitch);
        pParticle->mVel.x = -flCosPitch * sinf(flYaw) * flSpeed;
        pParticle->mVel.y = flCosPitch * cosf(flYaw) * flSpeed;
        pParticle->mVel.z = sinf(flPitch) * flSpeed;
        TransformVec3ByMat3VU0(&pParticle->mVel.x, &mWorldXfm[0][0], &pParticle->mVel.x);

        pParticle->mPos.x = RandomInRange(mPosLow.x, mPosHigh.x);
        pParticle->mPos.y = RandomInRange(mPosLow.y, mPosHigh.y);
        pParticle->mPos.z = RandomInRange(mPosLow.z, mPosHigh.z);
        TransformPointByWorldXfm(pParticle->mPos, mWorldXfm);

        if (mBubble != 0) {
            pParticle->mBubbleFrequency = kTwoPi / RandomInRange(mBubblePeriod.x, mBubblePeriod.y);
            pParticle->mBubblePhase = RandomInRange(0.0f, kTwoPi);
            const float flAngle = RandomInRange(0.0f, kTwoPi);
            Vector3 direction;
            direction.x = sinf(flAngle);
            direction.y = 0.0f;
            direction.z = cosf(flAngle);
            direction.w = 1.0f;
            Vector3 bubbleSize;
            bubbleSize.w = 1.0f;
            Vec3Scale(&direction.x, RandomInRange(mBubbleSize.x, mBubbleSize.y), &bubbleSize.x);
            pParticle->mBubbleSize = bubbleSize;

            Vector3 offset;
            offset.w = 1.0f;
            Vec3Scale(&pParticle->mBubbleSize.x, sinf(pParticle->mBubblePhase), &offset.x);
            AddVec3(&pParticle->mPos.x, &offset.x, &pParticle->mPos.x);
            pParticle->mBubblePhase -= mFilteredFrame * pParticle->mBubbleFrequency;
        }

        // Yes, the history runs through the quadwords after mPrevPos, as in UpdateParticles().
        Vector3 *pHistory = &pParticle->mPrevPos;
        for (int nIndex = 0; nIndex < mLineLength; ++nIndex) {
            pHistory[nIndex] = pParticle->mPos;
        }

        RandomizeColorAndSize(pParticle);
        pParticle->mColVel.r = RandomInRange(mEndColorLow.r, mEndColorHigh.r);
        pParticle->mColVel.g = RandomInRange(mEndColorLow.g, mEndColorHigh.g);
        pParticle->mColVel.b = RandomInRange(mEndColorLow.b, mEndColorHigh.b);
        pParticle->mColVel.a = RandomInRange(mEndColorLow.a, mEndColorHigh.a);
        SubColor(pParticle->mColVel, pParticle->mCol, pParticle->mColVel);
        // The binary scales through the routine at 0x00453e08.
        const float flRate = 1.0f / (pParticle->mDeathFrame - pParticle->mBirthFrame);
        pParticle->mColVel.r *= flRate;
        pParticle->mColVel.g *= flRate;
        pParticle->mColVel.b *= flRate;
        pParticle->mColVel.a *= flRate;

        mEmitAccumulator -= 1.0f;
    }
}

// 0x0052c530
void ParticleSys::RandomizeColorAndSize(Particle *pParticle) {
    pParticle->mCol.r = RandomInRange(mStartColorLow.r, mStartColorHigh.r);
    pParticle->mCol.g = RandomInRange(mStartColorLow.g, mStartColorHigh.g);
    pParticle->mCol.b = RandomInRange(mStartColorLow.b, mStartColorHigh.b);
    pParticle->mCol.a = RandomInRange(mStartColorLow.a, mStartColorHigh.a);
    pParticle->mSize = RandomInRange(mSizeLow, mSizeHigh);
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
