#include "app/tnlgem.h"

#include <iostream>

#include "app/apptunnel.h"
#include "app/tnlgemeffectkind.h"
#include "app/tnlgemmanager.h"
#include "app/tnlgemmeshkind.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "math/color.h"
#include "rnd/multimesh.h"
#include "rnd/particle.h"
#include "rnd/particlesys.h"
#include "rnd/tunnel.h"

namespace {

// mExpireFrame of a gem no later gem has replaced.
constexpr float kNeverExpire = 1.0e9f;

// Red component of the purple particle.
constexpr float kPurpleRed = 0.65f;

// Colour indices, numbered as TnlColorIndexFromName() numbers the player colours.
enum ColorIndex {
    kColorIndexGreen = 1,
    kColorIndexRed = 2,
    kColorIndexYellow = 3,
    kColorIndexPurple = 4,
};

inline Color ParticleColor(int nColor) {
    switch (nColor) {
    case kColorIndexGreen:
        return Color{0.0f, 1.0f, 0.0f, 1.0f};
    case kColorIndexRed:
        return Color{1.0f, 0.0f, 0.0f, 1.0f};
    case kColorIndexYellow:
        return Color{1.0f, 1.0f, 0.0f, 1.0f};
    case kColorIndexPurple:
        return Color{kPurpleRed, 0.0f, 1.0f, 1.0f};
    default:
        return Color{1.0f, 1.0f, 1.0f, 1.0f};
    }
}

inline void GetGemXfm(const TnlGem &gem, Transform &xfm) {
    PadTransformRows(xfm);
    GetCachedTunnelObject()->GetRingXfm(gem.mTrack, &xfm, gem.mFrame, gem.mBlend);
}

} // namespace

// 0x00415790
TnlGem::TnlGem(char nKind,
               char nTrack,
               char nColor,
               bool bFlash,
               float flFrame,
               float flBlend,
               float flAppearFrame)
    : mKind(nKind), mTrack(nTrack), mState(bFlash ? kStateFlashPending : kStateNone),
      mColor(nColor), mFrame(flFrame), mBlend(flBlend), mAppearFrame(flAppearFrame),
      mExpireFrame(kNeverExpire), mMeshKind(nullptr), mEffectKind(nullptr) {
}

// 0x00411e38
float TnlGem::Place(TnlGemManager *pManager, float flFrame) {
    if (!(mKind & TnlGemManager::kEffectKindBase)) {
        if (!mMeshKind) {
            Transform xfm;
            GetGemXfm(*this, xfm);
            mMeshKind = pManager->GetMeshKind(mKind);
            auto &transforms = mMeshKind->mMesh->GetTransforms();
            mInstance = transforms.insert(transforms.end(), xfm);
        }
        TnlGemMeshKind *pKind = mMeshKind;
        while (pKind && !((flFrame + pKind->mLodDistance) < mFrame)) {
            pKind = pKind->mNext;
        }
        if (pKind && (pKind != mMeshKind)) {
            auto &transforms = pKind->mMesh->GetTransforms();
            transforms.splice(transforms.end(), mMeshKind->mMesh->GetTransforms(), mInstance);
            mMeshKind = pKind;
        }
        return mMeshKind->GetCost();
    }
    if (mEffectKind) {
        return mEffectKind->mCost;
    }
    mEffectKind = pManager->GetEffectKind(mKind);
    mParticle = mEffectKind->mParticleSys->AllocParticle();
    if (!mParticle) {
        mEffectKind = nullptr;
        return 0.0f;
    }
    Transform xfm;
    GetGemXfm(*this, xfm);
    mParticle->mCol = ParticleColor(mColor);
    mParticle->mPos = xfm.mTranslation;
    mParticle->mSize = mEffectKind->mParticleSys->mSizeLow;
    return mEffectKind->mCost;
}

// 0x004157e8
void TnlGem::Release() {
    if (mMeshKind) {
        mMeshKind->mMesh->GetTransforms().erase(mInstance);
        mMeshKind = nullptr;
    } else if (mEffectKind) {
        mEffectKind->mParticleSys->FreeParticle(mParticle);
        mEffectKind = nullptr;
    }
}

// 0x00415868
void TnlGem::Flash(AppTunnel *pTunnel) {
    if (mMeshKind) {
        pTunnel->StartGemFlash(mInstance->mTranslation);
    } else if (mEffectKind) {
        pTunnel->StartGemFlash(mParticle->mPos);
    }
}

// 0x00411d30
std::ostream &operator<<(std::ostream &stream, const TnlGem &gem) {
    stream << '[' << gem.mKind << ' ' << gem.mTrack << ' ' << gem.mFrame << ' ' << gem.mBlend
           << " (" << gem.mAppearFrame << ' ' << gem.mExpireFrame << ")]";
    return stream;
}
