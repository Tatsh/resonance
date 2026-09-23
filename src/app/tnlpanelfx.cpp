#include "app/tnlpanelfx.h"

#include <algorithm>

#include "app/tnlname.h"
#include "app/tunnelcache.h"
#include "math/transform.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

// Phase frame the constructor writes, far beyond a song's length.
constexpr float kNoFrame = 1e9f;

constexpr int kFramesPerBar = 1920;
constexpr int kHalfBarFrames = 960;

// Weight of the next ring's translation in the rise offset, the point halfway between rings.
constexpr float kRingBlend = 0.5f;

constexpr float kRiseFrames = 200.0f;
constexpr float kHoldFrames = 400.0f;
constexpr float kFadeFrames = 200.0f;

// Report how far a phase that began at flStart has run, clamped at 1.
inline float PhaseFraction(float flFrame, float flStart, float flDuration) {
    float flFraction = (flFrame - flStart) / flDuration;
    if (1.0f <= flFraction) {
        flFraction = 1.0f;
    }
    return flFraction;
}

inline Vector3 Scaled(const Vector3 &vector, float flScale) {
    Vector3 result;
    result.w = 1.0f;
    Vec3Scale(&vector.x, flScale, &result.x);
    return result;
}

} // namespace

// 0x0043c688
TnlPanelFX::TnlPanelFX(int nIndex) : mStateFrame(kNoFrame), mState(kStateIdle), mIndex(nIndex) {
    mOffset.w = 1.0f;

    mMesh = Rnd::NewMeshThroughHook(HxStr("pnl") + NextAppTunnelName());
    mMat = nullptr;
    mView = dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("tnl transparent")));
    mMesh->mZFunc = Rnd::Mesh::kZFuncLess;
    mMesh->mZMode = Rnd::Mesh::kZModeZReadOnly;
    mMesh->SetShowing(0);
    mView->AddDraw(mMesh, nullptr);

    const HxStr matName(FormatString("tunnel erase%d", nIndex));
    mMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(matName));
    if (mMat == nullptr) {
        Rnd::Mat *pTemplate = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("tunnel erase0")));
        mMat = Rnd::NewMatThroughHook(matName);
        mMat->Copy(pTemplate, 0);
    }
}

// 0x0043cb78
void TnlPanelFX::Start(int nRing, int nSlice, int nForward) {
    mDirection = nForward != 0 ? 1.0f : -1.0f;
    mMat->mStages.front().mBlend = Rnd::Mat::kBlendModeSrcAlpha;
    mMat->SetAlpha(0.0f);

    Rnd::Mesh *pSection = GetCachedTunnelObject()->GetRingSection(nRing, nSlice);
    mMesh->SetFacesOwner(pSection->mFacesOwner);
    mMesh->Sync();
    mMesh->mVertsOwner->mVerts = pSection->mVertsOwner->mVerts;
    mMesh->SyncAll();
    mMesh->SetMaterial(mMat);

    const float flFrame = static_cast<float>(nSlice * kFramesPerBar + kHalfBarFrames);
    Transform xfm;
    GetCachedTunnelObject()->GetRingXfm(nRing, &xfm, flFrame, kRingBlend);
    mOffset = xfm.mTranslation;
    GetCachedTunnelObject()->GetPathXfm(&xfm, flFrame);
    Vec3Sub(&mOffset.x, &xfm.mTranslation.x, &mOffset.x);

    mMesh->SetShowing(1);
    mState = kStateRise;
}

// 0x0043cd38
void TnlPanelFX::Update(float flFrame) {
    switch (mState) {
    case kStateIdle:
        mStateFrame = flFrame;
        break;
    case kStateRise: {
        const float flFraction = PhaseFraction(flFrame, mStateFrame, kRiseFrames);
        const Vector3 origin = Scaled(Scaled(mOffset, 1.0f - flFraction), mDirection);
        mMesh->SetOrigin(&origin.x);
        mMesh->UpdateWorldXfm(nullptr, 0);
        mMat->SetAlpha(std::min(1.0f, flFraction + flFraction));
        mMat->mStages.front().mBlend = Rnd::Mat::kBlendModeSrcAlpha;
        if (1.0f <= flFraction) {
            mStateFrame = flFrame;
            mState = kStateHold;
        }
        break;
    }
    case kStateHold: {
        const float flFraction = PhaseFraction(flFrame, mStateFrame, kHoldFrames);
        mMat->SetAlpha(std::max(0.0f, 1.0f - flFraction));
        mMat->mStages.front().mBlend = Rnd::Mat::kBlendModeSrcAlphaAdd;
        if (1.0f <= flFraction) {
            mStateFrame = flFrame;
            mState = kStateFade;
        }
        break;
    }
    case kStateFade: {
        const float flFraction = PhaseFraction(flFrame, mStateFrame, kFadeFrames);
        mMat->SetAlpha(std::max(0.0f, 1.0f - flFraction));
        mMat->mStages.front().mBlend = Rnd::Mat::kBlendModeSrcAlpha;
        if (1.0f <= flFraction) {
            mMesh->SetFacesOwner(mMesh);
            mMesh->SetShowing(0);
            mState = kStateIdle;
        }
        break;
    }
    }
}
