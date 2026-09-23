#include "rnd/particlesys.h"

#include <list>
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
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/object.h"
#include "rnd/particle.h"
#include "rnd/stream.h"

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

// The revision Save() writes and the newest Load() accepts, and the revisions at which Load() began
// reading each later part of the record.
constexpr int kParticleSysRevision = 6;
constexpr int kRevisionBaseRecords = 1;
constexpr int kRevisionSingleCollidePlane = 2;
constexpr int kRevisionLineLength = 3;
constexpr int kRevisionBubble = 4;
constexpr int kRevisionReadZ = 5;
constexpr int kRevisionParticlesOwner = 6;

// Load() accepts no line length above this, whatever the file stores.
constexpr int kMaxLoadedLineLength = 1;

// The stream helpers below reproduce the inline stream operators the binary open-codes.

inline void WriteFloat(Stream &stream, float flValue) {
    stream.Write(&flValue, sizeof(flValue));
}

inline void WriteInt(Stream &stream, int nValue) {
    stream.Write(&nValue, sizeof(nValue));
}

// A flag goes out as its low byte.
inline void WriteFlag(Stream &stream, int bValue) {
    const unsigned char chValue = static_cast<unsigned char>(bValue);
    stream.WriteBytes(&chValue, sizeof(chValue));
}

inline int ReadFlag(Stream &stream) {
    unsigned char chValue = 0;
    stream.ReadBytes(&chValue, sizeof(chValue));
    return chValue != 0;
}

// A plane is stored as its point nearest the origin followed by its normal.
inline void WritePlane(Stream &stream, const Plane &plane) {
    WriteFloat(stream, -plane.d * plane.a);
    WriteFloat(stream, -plane.d * plane.b);
    WriteFloat(stream, -plane.d * plane.c);
    WriteFloat(stream, plane.a);
    WriteFloat(stream, plane.b);
    WriteFloat(stream, plane.c);
}

inline void ReadPlane(Stream &stream, Plane &plane) {
    Vector3 point;
    point.w = 1.0f;
    stream.Read(&point.x, sizeof(point.x))
        .Read(&point.y, sizeof(point.y))
        .Read(&point.z, sizeof(point.z));
    stream.Read(&plane.a, sizeof(plane.a))
        .Read(&plane.b, sizeof(plane.b))
        .Read(&plane.c, sizeof(plane.c));
    plane.d = -(point.x * plane.a + point.y * plane.b + point.z * plane.c);
}

// 0x00529ee0
Stream &operator>>(Stream &stream, std::list<Plane> &planes) {
    int nCount = 0;
    stream.Read(&nCount, sizeof(nCount));
    planes.resize(nCount);
    for (Plane &plane : planes) {
        ReadPlane(stream, plane);
    }
    return stream;
}

inline void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.WriteBytes(&chTerminator, sizeof(chTerminator));
        return;
    }
    stream.WriteBytes(pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString,
                      pObject->mName.mLen + 1);
}

template <class T>
inline void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(g_manager.Find(name));
}

// DumpText() lists the particles only at this dump level or above.
constexpr int kParticleListDumpLevel = 2;

// The print helpers below reproduce the formatting the binary open-codes at every dump site.

inline void PrintRange(FailSink &sink, float flLow, float flHigh) {
    sink.Print("(x:")->Format("%.2f", flLow)->Print(" y:")->Format("%.2f", flHigh)->Print(")");
}

inline void PrintVector3(FailSink &sink, const Vector3 &vec) {
    sink.Print("(x:")
        ->Format("%.2f", vec.x)
        ->Print(" y:")
        ->Format("%.2f", vec.y)
        ->Print(" z:")
        ->Format("%.2f", vec.z)
        ->Print(")");
}

inline void PrintColor(FailSink &sink, const Color &color) {
    sink.Print("(r:")
        ->Format("%.2f", color.r)
        ->Print(" g:")
        ->Format("%.2f", color.g)
        ->Print(" b:")
        ->Format("%.2f", color.b)
        ->Print(" a:")
        ->Format("%.2f", color.a)
        ->Print(")");
}

inline void PrintPlane(FailSink &sink, const Plane &plane) {
    sink.Print("(a:")
        ->Format("%.2f", plane.a)
        ->Print(" b:")
        ->Format("%.2f", plane.b)
        ->Print(" c:")
        ->Format("%.2f", plane.c)
        ->Print(" d:")
        ->Format("%.2f", plane.d)
        ->Print(")");
}

inline void PrintBool(FailSink &sink, int bValue) {
    sink.Print(bValue != 0 ? "true" : "false");
}

inline void PrintObjectRef(FailSink &sink, const Object *pObject) {
    if (pObject == nullptr) {
        sink.Print("no object");
        return;
    }
    sink.Format("\"%s\"", pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString);
}

// 0x00526308
FailSink &operator<<(FailSink &sink, const Particle &particle) {
    sink.Print("\n\tpos:");
    PrintVector3(sink, particle.mPos);
    sink.Print("\n\tprevPos:");
    PrintVector3(sink, particle.mPrevPos);
    sink.Print("\n\tvel:");
    PrintVector3(sink, particle.mVel);
    sink.Print("\n\tcol:");
    PrintColor(sink, particle.mCol);
    sink.Print("\n\tcolVel:");
    PrintColor(sink, particle.mColVel);
    // Yes, the binary writes "size:" with no separator after the colour rate.
    sink.Print("size:")
        ->Format("%.2f", particle.mSize)
        ->Print(" deathFrame:")
        ->Format("%.2f", particle.mDeathFrame)
        ->Print(" birthFrame:")
        ->Format("%.2f", particle.mBirthFrame);
    return sink;
}

// 0x00529c30
FailSink &operator<<(FailSink &sink, const std::vector<Particle> &particles) {
    sink.Print("(size:")->Format("%u", static_cast<unsigned>(particles.size()))->Print(")");
    for (std::vector<Particle>::const_iterator it = particles.begin(); it != particles.end();
         ++it) {
        *sink.Print("\n")->Format("%d", static_cast<int>(it - particles.begin()))->Print("\t")
            << *it;
    }
    return sink;
}

// Signed distance of a point from a plane. UpdateParticles() evaluates it twice.
inline float DistanceToPlane(const Plane &plane, const Vector3 &point) {
    return plane.a * point.x + plane.b * point.y + plane.c * point.z + plane.d;
}

} // namespace

// 0x0071aef0
HxStr g_particleSysClassName("ParticleSys");

// 0x0089dfb8
int g_nParticleSysLoadRevision;

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

// 0x00521f40
void ParticleSys::DumpText(FailSink &sink) {
    Object::DumpText(sink);
    Animatable::DumpText(sink);
    Transformable::DumpText(sink);
    Drawable::DumpText(sink);
    if (sink.mDumpLevel <= 0) {
        return;
    }

    sink.Print("[ParticleSys]\n");
    sink.Print("life:");
    PrintRange(sink, mLife.x, mLife.y);
    sink.Print(" posLow:");
    PrintVector3(sink, mPosLow);
    sink.Print(" posHigh:");
    PrintVector3(sink, mPosHigh);
    sink.Print("\n");

    sink.Print("speed:");
    PrintRange(sink, mSpeed.x, mSpeed.y);
    sink.Print(" pitch:");
    PrintRange(sink, mPitch.x, mPitch.y);
    sink.Print(" yaw:");
    PrintRange(sink, mYaw.x, mYaw.y);
    sink.Print("\n");

    sink.Print("emitRate:");
    PrintRange(sink, mEmitRateLow, mEmitRateHigh);
    sink.Print(" size:");
    PrintRange(sink, mSizeLow, mSizeHigh);
    sink.Print("\n");

    sink.Print("startColorLow:");
    PrintColor(sink, mStartColorLow);
    sink.Print("\n");
    sink.Print("startColorHigh:");
    PrintColor(sink, mStartColorHigh);
    sink.Print("\n");
    sink.Print("endColorLow:");
    PrintColor(sink, mEndColorLow);
    sink.Print("\n");
    sink.Print("endColorHigh:");
    PrintColor(sink, mEndColorHigh);
    sink.Print("\n");

    sink.Print("collide:");
    PrintBool(sink, mCollide);
    sink.Print(" collidePlane:");
    PrintPlane(sink, mCollidePlane);
    sink.Print("\n");

    sink.Print("force:");
    PrintVector3(sink, mForce);
    sink.Print(" mat:");
    PrintObjectRef(sink, mMat);
    sink.Print(" mode:");
    PrintParticleMode(sink, mMode);
    sink.Print("\n");

    sink.Print("numParticles:")->Format("%u", static_cast<unsigned>(mParticles.size()));
    sink.Print(" lineLength:")->Format("%d", mLineLength);
    sink.Print("\n");

    sink.Print("bubblePeriod:");
    PrintRange(sink, mBubblePeriod.x, mBubblePeriod.y);
    sink.Print(" bubbleSize:");
    PrintRange(sink, mBubbleSize.x, mBubbleSize.y);
    sink.Print("\n");

    sink.Print("bubble:");
    PrintBool(sink, mBubble);
    sink.Print(" readZ:");
    PrintBool(sink, mReadZ);
    sink.Print(" particlesOwner:");
    PrintObjectRef(sink, mParticlesOwner);
    sink.Print("\n");

    if (sink.mDumpLevel >= kParticleListDumpLevel) {
        (*sink.Print("particles:") << mParticles).Print("\n");
    }
}

// 0x00522d60
void ParticleSys::Save(Stream &stream) {
    WriteInt(stream, kParticleSysRevision);
    Animatable::Save(stream);
    Transformable::Save(stream);
    Drawable::Save(stream);

    WriteFloat(stream, mLife.x);
    WriteFloat(stream, mLife.y);
    WriteFloat(stream, mPosLow.x);
    WriteFloat(stream, mPosLow.y);
    WriteFloat(stream, mPosLow.z);
    WriteFloat(stream, mPosHigh.x);
    WriteFloat(stream, mPosHigh.y);
    WriteFloat(stream, mPosHigh.z);
    WriteFloat(stream, mSpeed.x);
    WriteFloat(stream, mSpeed.y);
    WriteFloat(stream, mPitch.x);
    WriteFloat(stream, mPitch.y);
    WriteFloat(stream, mYaw.x);
    WriteFloat(stream, mYaw.y);
    WriteFloat(stream, mEmitRateLow);
    WriteFloat(stream, mEmitRateHigh);
    WriteFloat(stream, mSizeLow);
    WriteFloat(stream, mSizeHigh);
    const Color *apColors[] = {&mStartColorLow, &mStartColorHigh, &mEndColorLow, &mEndColorHigh};
    for (const Color *pColor : apColors) {
        WriteFloat(stream, pColor->r);
        WriteFloat(stream, pColor->g);
        WriteFloat(stream, pColor->b);
        WriteFloat(stream, pColor->a);
    }

    WriteFlag(stream, mCollide);
    WritePlane(stream, mCollidePlane);
    WriteFloat(stream, mForce.x);
    WriteFloat(stream, mForce.y);
    WriteFloat(stream, mForce.z);
    WriteObjectRef(stream, mMat);
    WriteInt(stream, mMode);
    WriteInt(stream, static_cast<int>(mParticles.size()));
    WriteInt(stream, mLineLength);
    WriteFloat(stream, mBubblePeriod.x);
    WriteFloat(stream, mBubblePeriod.y);
    WriteFloat(stream, mBubbleSize.x);
    WriteFloat(stream, mBubbleSize.y);
    WriteFlag(stream, mBubble);
    WriteFlag(stream, mReadZ);
    WriteObjectRef(stream, mParticlesOwner);
}

// 0x00523718
void ParticleSys::Load(Stream &stream) {
    stream.Read(&g_nParticleSysLoadRevision, sizeof(g_nParticleSysLoadRevision));
    if (g_nParticleSysLoadRevision > kParticleSysRevision) {
        g_failSink.Report("Can't load new ParticleSys\n");
        return;
    }
    if (g_nParticleSysLoadRevision >= kRevisionBaseRecords) {
        Animatable::Load(stream);
        Transformable::Load(stream);
        Drawable::Load(stream);
    }
    RemoveObjectRefs();

    float *apFloatFields[] = {&mLife.x,
                              &mLife.y,
                              &mPosLow.x,
                              &mPosLow.y,
                              &mPosLow.z,
                              &mPosHigh.x,
                              &mPosHigh.y,
                              &mPosHigh.z,
                              &mSpeed.x,
                              &mSpeed.y,
                              &mPitch.x,
                              &mPitch.y,
                              &mYaw.x,
                              &mYaw.y,
                              &mEmitRateLow,
                              &mEmitRateHigh,
                              &mSizeLow,
                              &mSizeHigh};
    for (float *pflValue : apFloatFields) {
        stream.Read(pflValue, sizeof(*pflValue));
    }
    Color *apColors[] = {&mStartColorLow, &mStartColorHigh, &mEndColorLow, &mEndColorHigh};
    for (Color *pColor : apColors) {
        stream.Read(&pColor->r, sizeof(pColor->r));
        stream.Read(&pColor->g, sizeof(pColor->g));
        stream.Read(&pColor->b, sizeof(pColor->b));
        stream.Read(&pColor->a, sizeof(pColor->a));
    }

    if (g_nParticleSysLoadRevision < kRevisionSingleCollidePlane) {
        // The oldest revision stored a list of collision planes and no flag. The list is read and
        // discarded, and mCollide and mCollidePlane retain their values.
        std::list<Plane> collidePlanes;
        stream >> collidePlanes;
    } else {
        mCollide = ReadFlag(stream);
        ReadPlane(stream, mCollidePlane);
    }
    stream.Read(&mForce.x, sizeof(mForce.x));
    stream.Read(&mForce.y, sizeof(mForce.y));
    stream.Read(&mForce.z, sizeof(mForce.z));
    ReadObjectRef(stream, mMat);

    int nMode = 0;
    stream.Read(&nMode, sizeof(nMode));
    mMode = static_cast<Mode>(nMode);
    int nParticles = 0;
    stream.Read(&nParticles, sizeof(nParticles));
    mParticles.resize(nParticles);

    if (g_nParticleSysLoadRevision >= kRevisionLineLength) {
        stream.Read(&mLineLength, sizeof(mLineLength));
        if (mLineLength > kMaxLoadedLineLength) {
            mLineLength = kMaxLoadedLineLength; // Yes, the binary discards any longer line.
        }
    }
    if (g_nParticleSysLoadRevision >= kRevisionBubble) {
        stream.Read(&mBubblePeriod.x, sizeof(mBubblePeriod.x));
        stream.Read(&mBubblePeriod.y, sizeof(mBubblePeriod.y));
        stream.Read(&mBubbleSize.x, sizeof(mBubbleSize.x));
        stream.Read(&mBubbleSize.y, sizeof(mBubbleSize.y));
        mBubble = ReadFlag(stream);
    }
    if (g_nParticleSysLoadRevision >= kRevisionReadZ) {
        mReadZ = ReadFlag(stream);
    }
    if (g_nParticleSysLoadRevision >= kRevisionParticlesOwner) {
        ReadObjectRef(stream, mParticlesOwner);
    }

    // Every revision this build accepts reaches this test.
    if (g_nParticleSysLoadRevision <= kParticleSysRevision && mParticlesOwner != this) {
        mParticles.clear();
    }
    AddObjectRefs();
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
