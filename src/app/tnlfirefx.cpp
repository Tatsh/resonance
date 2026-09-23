#include "app/tnlfirefx.h"

#include <cmath>
#include <cstring>

#include "app/tunnelcache.h"
#include "math/color.h"
#include "math/transform.h"
#include "math/transformops.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/particlesys.h"
#include "rnd/transanim.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

// Reference frame the constructor writes, far beyond a song's length.
constexpr float kNoFrame = 1e9f;

// Marks the reference frame as not yet recorded, and the end frame as not yet set.
constexpr float kUnset = -1.0f;

// Index of the objects that also have a second system.
constexpr int kIndexWithAltEmitter = -1;

// One unit in the last place below the float nearest pi, as the binary stores it.
constexpr float kPi = 3.1415925f;
constexpr float kEighth = 0.125f;

constexpr float kPathFramesPerFrame = 4.0f;

inline void SetSystemColors(Rnd::ParticleSys *pSys, const Color &color) {
    const Color white{1.0f, 1.0f, 1.0f, 1.0f};
    Color fade{color.r, color.g, color.b, 1.0f};
    pSys->mStartColorLow = white;
    pSys->mStartColorHigh = fade;
    fade.a = 0.0f;
    pSys->mEndColorLow = fade;
    pSys->mEndColorHigh = fade;
}

inline void SetSystemBasis(Rnd::ParticleSys *pSys, const Vector3 *pBasis, std::size_t nSize) {
    std::memcpy(pSys->mLocalXfm, pBasis, nSize);
    pSys->mDirty = 1;
}

} // namespace

// 0x0043d368
TnlFireFX::TnlFireFX(const HxStr &name, int nIndex)
    : mPathStartFrame(0.0f), mPathEndFrame(kUnset), mReferenceFrame(kNoFrame), mActive(0),
      mIndex(nIndex) {
    mPath = dynamic_cast<Rnd::TransAnim *>(Rnd::g_manager.Find(HxStr("fx.path")));

    HxStr viewName(name);
    viewName += ".view";
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(viewName));

    HxStr sysName(name);
    sysName += ".ps";
    mEmitter.Attach(dynamic_cast<Rnd::ParticleSys *>(Rnd::g_manager.Find(sysName)));

    if (nIndex == kIndexWithAltEmitter) {
        HxStr altName(name);
        altName += "a.ps";
        mAltEmitter.Attach(dynamic_cast<Rnd::ParticleSys *>(Rnd::g_manager.Find(altName)));
    }
}

// 0x0043d8c0
int TnlFireFX::Start(float flPathStart,
                     int nIndex,
                     int nSlot,
                     const Color &color,
                     const Color &altColor,
                     float flPathEnd) {
    if (mActive != 0 || nIndex != mIndex) {
        return 0;
    }

    mReferenceFrame = kUnset;
    mPathEndFrame = flPathEnd;
    mPathStartFrame = flPathStart;
    if (mEmitter.GetParticleSys() != nullptr) {
        SetSystemColors(mEmitter.GetParticleSys(), color);
    }
    if (mAltEmitter.GetParticleSys() != nullptr) {
        SetSystemColors(mAltEmitter.GetParticleSys(), altColor);
    }

    const float flAngle = static_cast<float>(-nSlot * 2) * kPi * kEighth;
    const float flCos = cosf(flAngle);
    const float flSin = sinf(flAngle);
    Vector3 basis[] = {
        {flCos, 0.0f, -flSin, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {flSin, 0.0f, flCos, 1.0f},
    };
    // The first system is not tested for null here, unlike everywhere else.
    SetSystemBasis(mEmitter.GetParticleSys(), basis, sizeof(basis));
    if (mAltEmitter.GetParticleSys() != nullptr) {
        SetSystemBasis(mAltEmitter.GetParticleSys(), basis, sizeof(basis));
    }

    Transform pathXfm;
    GetCachedTunnelObject()->GetPathXfm(&pathXfm, flPathStart);
    MultiplyMat3VU0(&basis[0].x, &pathXfm.mBasisX.x, &basis[0].x);
    mEmitter.RotateForce(&basis[0].x);
    mAltEmitter.RotateForce(&basis[0].x);

    mEmitter.Restart();
    mAltEmitter.Restart();
    mActive = 1;
    return 1;
}

// 0x004565a8
void TnlFireFX::SetFrame(float flFrame, float flViewFrame) {
    mView->SetFrame(flViewFrame);
    if (mReferenceFrame == kUnset) {
        mReferenceFrame = flFrame;
    }
    if (mActive == 0) {
        return;
    }

    const float flElapsed = flFrame - mReferenceFrame;
    mPath->SetTrans(mView);
    const float flPathFrame = mPathStartFrame + flElapsed * kPathFramesPerFrame;
    mPath->SetFrame(flPathFrame);
    if (mPathEndFrame < flPathFrame) {
        mEmitter.Stop();
        mAltEmitter.Stop();
        mActive = 0;
    }
}
