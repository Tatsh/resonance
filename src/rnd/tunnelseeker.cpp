#include "rnd/tunnelseeker.h"

#include <algorithm>
#include <math.h>
#include <string.h>

#include "rnd/mesh.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"

namespace Rnd {

namespace {

constexpr float kDefaultTransFrameOffset = -500.0f;

} // namespace

// 0x00477768
TunnelSeeker::TunnelSeeker()
    : mTrans(nullptr), mTargetRing(0), mMeshFrameOffset(0.0f),
      mTransFrameOffset(kDefaultTransFrameOffset), mLookFrameOffset(0.0f), mMesh(nullptr),
      mTunnel(nullptr), mLane(0.0f), mFrame(0.0f) {
}

// 0x004777c0
void TunnelSeeker::ReleaseRefs() {
    if (mTrans != nullptr) {
        mTrans->RemoveRef(mTunnel);
    }
    if (mMesh != nullptr) {
        mMesh->RemoveRef(mTunnel);
    }
    mStrip.Clear();
}

// 0x00477830
void TunnelSeeker::SetTunnel(Tunnel *pTunnel, int nIndex) {
    mTunnel = pTunnel;
    mFrame = pTunnel->mFilteredFrame;
    if (mTrans != nullptr) {
        mTrans->AddRef(pTunnel);
    }
    if (mMesh != nullptr) {
        mMesh->AddRef(mTunnel);
    }
    mStrip.Build(mTunnel, this, nIndex);
}

// 0x00477a48
void TunnelSeeker::SetTrans(Transformable *pTrans) {
    if (mTrans != nullptr) {
        mTrans->RemoveRef(mTunnel);
    }
    mTrans = pTrans;
    if (pTrans != nullptr) {
        pTrans->AddRef(mTunnel);
    }
}

// 0x00477ab8
void TunnelSeeker::SetTargetRing(int nRing) {
    mTargetRing = nRing;
}

// 0x00477ac0
void TunnelSeeker::SetMesh(Mesh *pMesh) {
    if (mMesh != nullptr) {
        mMesh->RemoveRef(mTunnel);
    }
    mMesh = pMesh;
    if (pMesh != nullptr) {
        pMesh->AddRef(mTunnel);
    }
}

// 0x00477b30
void TunnelSeeker::SetMeshFrameOffset(float flOffset) {
    mMeshFrameOffset = flOffset;
}

// 0x00477b38
void TunnelSeeker::SetTransFrameOffset(float flOffset) {
    mTransFrameOffset = flOffset;
}

// 0x00477b40
void TunnelSeeker::SetLookFrameOffset(float flOffset) {
    mLookFrameOffset = flOffset;
}

// 0x00477b48
void TunnelSeeker::SetRange(int nFirstSlice, int nSliceCount, int nRing) {
    mStrip.SetRange(nFirstSlice, nSliceCount, nRing);
}

// 0x00477b68
void TunnelSeeker::SetColor(const Color &color) {
    mStrip.SetColor(color);
}

// 0x00477b88
void TunnelSeeker::SetMat(Mat *pMat) {
    mStrip.SetMat(pMat);
}

// 0x00477bd8
void TunnelSeeker::DrawSection(int nSlice, float flScreenSize) {
    mStrip.DrawSection(nSlice, flScreenSize);
}

// 0x00477bf8
void TunnelSeeker::DrawMesh() {
    if (mMesh != nullptr) {
        mMesh->Draw();
    }
}

// 0x0046e6a0
float TunnelSeeker::UpdateLane() {
    const float flTarget = static_cast<float>(mTargetRing);
    if (mLane != flTarget) {
        const float flHalfRings = static_cast<float>(mTunnel->mRingCount) * 0.5f;
        const float flLow = -flHalfRings;
        const float flSpan = flHalfRings - flLow;
        float flWrapped = fmodf(flTarget - mLane - flLow, flSpan);
        if (flWrapped < 0.0f) {
            flWrapped += flSpan;
        }
        const float flDistance = flLow + flWrapped;
        const float flAbsDistance = fabsf(flDistance);
        const float flStep = fabsf(mTunnel->mFilteredFrame - mFrame) / mTunnel->mUnknown64 *
                             std::max(flAbsDistance, 1.0f);
        if (flAbsDistance < flStep) {
            mLane = static_cast<float>(mTargetRing);
        } else if (0.0f < flDistance) {
            mLane += flStep;
        } else {
            mLane -= flStep;
        }
    }
    mFrame = mTunnel->mFilteredFrame;
    return mLane;
}

// 0x00477c20
void TunnelSeeker::SetTransXfm(const Transform &xfm) {
    if (mTrans != nullptr) {
        memcpy(mTrans->mLocalXfm, &xfm, sizeof(mTrans->mLocalXfm));
        mTrans->mDirty = 1;
    }
}

// 0x00477c58
void TunnelSeeker::SetMeshXfm(const Transform &xfm) {
    if (mMesh != nullptr) {
        memcpy(mMesh->mLocalXfm, &xfm, sizeof(mMesh->mLocalXfm));
        mMesh->mDirty = 1;
        mMesh->UpdateWorldXfm(nullptr, 0);
    }
}

// 0x004778c0
void TunnelSeeker::Replace(Object *pFrom, Object *pTo, Object *pReferrer) {
    if (mTrans == pFrom && mTrans != nullptr) {
        pFrom->RemoveRef(pReferrer);
        mTrans = dynamic_cast<Transformable *>(pTo);
        if (mTrans != nullptr) {
            mTrans->AddRef(pReferrer);
        }
    }
    if (mMesh == pFrom && mMesh != nullptr) {
        pFrom->RemoveRef(pReferrer);
        mMesh = dynamic_cast<Mesh *>(pTo);
        if (mMesh != nullptr) {
            mMesh->AddRef(pReferrer);
        }
    }
    mStrip.Replace(pFrom, pTo, pReferrer);
}

} // namespace Rnd
