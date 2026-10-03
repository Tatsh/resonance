#include "rnd/tunnelseeker.h"

#include <algorithm>
#include <math.h>
#include <string.h>

#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/stream.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"

namespace Rnd {

namespace {

constexpr float kDefaultTransFrameOffset = -500.0f;

// The first stream revision that stores the three frame offsets.
constexpr int kFrameOffsetRevision = 34;

void WriteObjectRef(Stream &stream, const Object *pObject) {
    if (pObject == nullptr) {
        const char chTerminator = '\0';
        stream.Write(&chTerminator, 1);
        return;
    }
    const char *pszName = pObject->mName.mStr != nullptr ? pObject->mName.mStr : g_szEmptyString;
    stream.Write(pszName, pObject->mName.mLen + 1);
}

template <class T>
void ReadObjectRef(Stream &stream, T *&refOut) {
    HxStr name(nullptr);
    stream.ReadString(name);
    refOut = dynamic_cast<T *>(TheManager.Find(name));
}

} // namespace

// NTSC-U/C: 0x00477768, PAL: 0x004b53e0
TunnelSeeker::TunnelSeeker()
    : mTrans(nullptr), mTargetRing(0), mMeshFrameOffset(0.0f),
      mTransFrameOffset(kDefaultTransFrameOffset), mLookFrameOffset(0.0f), mMesh(nullptr),
      mTunnel(nullptr), mLane(0.0f), mFrame(0.0f) {
}

// NTSC-U/C: 0x004777c0, PAL: 0x004b5438
void TunnelSeeker::ReleaseRefs() {
    if (mTrans != nullptr) {
        mTrans->RemoveRef(mTunnel);
    }
    if (mMesh != nullptr) {
        mMesh->RemoveRef(mTunnel);
    }
    mStrip.Clear();
}

// NTSC-U/C: 0x00477830, PAL: 0x004b54a8
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

// NTSC-U/C: 0x00477a48, PAL: 0x004b56c0
void TunnelSeeker::SetTrans(Transformable *pTrans) {
    if (mTrans != nullptr) {
        mTrans->RemoveRef(mTunnel);
    }
    mTrans = pTrans;
    if (pTrans != nullptr) {
        pTrans->AddRef(mTunnel);
    }
}

// NTSC-U/C: 0x00477ab8, PAL: 0x004b5730
void TunnelSeeker::SetTargetRing(int nRing) {
    mTargetRing = nRing;
}

// NTSC-U/C: 0x00477ac0, PAL: 0x004b5738
void TunnelSeeker::SetMesh(Mesh *pMesh) {
    if (mMesh != nullptr) {
        mMesh->RemoveRef(mTunnel);
    }
    mMesh = pMesh;
    if (pMesh != nullptr) {
        pMesh->AddRef(mTunnel);
    }
}

// NTSC-U/C: 0x00477b30, PAL: 0x004b57a8
void TunnelSeeker::SetMeshFrameOffset(float flOffset) {
    mMeshFrameOffset = flOffset;
}

// NTSC-U/C: 0x00477b38, PAL: 0x004b57b0
void TunnelSeeker::SetTransFrameOffset(float flOffset) {
    mTransFrameOffset = flOffset;
}

// NTSC-U/C: 0x00477b40, PAL: 0x004b57b8
void TunnelSeeker::SetLookFrameOffset(float flOffset) {
    mLookFrameOffset = flOffset;
}

// NTSC-U/C: 0x00477b48, PAL: 0x004b57c0
void TunnelSeeker::SetRange(int nFirstSlice, int nSliceCount, int nRing) {
    mStrip.SetRange(nFirstSlice, nSliceCount, nRing);
}

// NTSC-U/C: 0x00477b68, PAL: 0x004b57e0
void TunnelSeeker::SetColor(const Color &color) {
    mStrip.SetColor(color);
}

// NTSC-U/C: 0x00477b88, PAL: 0x004b5800
void TunnelSeeker::SetMat(Mat *pMat) {
    mStrip.SetMat(pMat);
}

// NTSC-U/C: 0x00477bd8, PAL: 0x004b5850
void TunnelSeeker::DrawSection(int nSlice, float flScreenSize) {
    mStrip.DrawSection(nSlice, flScreenSize);
}

// NTSC-U/C: 0x00477bf8, PAL: 0x004b5870
void TunnelSeeker::DrawMesh() {
    if (mMesh != nullptr) {
        mMesh->Draw();
    }
}

// NTSC-U/C: 0x0046e6a0, PAL: 0x004ac280
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
        const float flStep = fabsf(mTunnel->mFilteredFrame - mFrame) / mTunnel->mLaneChangeFrames *
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

// NTSC-U/C: 0x00477c20, PAL: 0x004b5898
void TunnelSeeker::SetTransXfm(const Transform &xfm) {
    if (mTrans != nullptr) {
        memcpy(mTrans->mLocalXfm, &xfm, sizeof(mTrans->mLocalXfm));
        mTrans->mDirty = 1;
    }
}

// NTSC-U/C: 0x00477c58, PAL: 0x004b58d0
void TunnelSeeker::SetMeshXfm(const Transform &xfm) {
    if (mMesh != nullptr) {
        memcpy(mMesh->mLocalXfm, &xfm, sizeof(mMesh->mLocalXfm));
        mMesh->mDirty = 1;
        mMesh->UpdateWorldXfm(nullptr, 0);
    }
}

// NTSC-U/C: 0x004778c0, PAL: 0x004b5538
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

// NTSC-U/C: 0x0046df88, PAL: 0x004abaf8
void TunnelSeeker::Save(Stream &stream) const {
    WriteObjectRef(stream, mTrans);
    stream.WriteLE(&mTargetRing, sizeof(mTargetRing));
    WriteObjectRef(stream, mMesh);
    const char chSavedFlag = mSavedFlag;
    stream.Write(&chSavedFlag, 1)
        .WriteLE(&mLane, sizeof(mLane))
        .WriteLE(&mMeshFrameOffset, sizeof(mMeshFrameOffset))
        .WriteLE(&mTransFrameOffset, sizeof(mTransFrameOffset))
        .WriteLE(&mLookFrameOffset, sizeof(mLookFrameOffset));
    stream.WriteLE(&mStrip.mFirstSlice, sizeof(mStrip.mFirstSlice))
        .WriteLE(&mStrip.mSliceCount, sizeof(mStrip.mSliceCount))
        .WriteLE(&mStrip.mRing, sizeof(mStrip.mRing));
    WriteObjectRef(stream, mStrip.mMat);
    stream.WriteLE(&mStrip.mColor.r, sizeof(mStrip.mColor.r))
        .WriteLE(&mStrip.mColor.g, sizeof(mStrip.mColor.g))
        .WriteLE(&mStrip.mColor.b, sizeof(mStrip.mColor.b))
        .WriteLE(&mStrip.mColor.a, sizeof(mStrip.mColor.a));
}

// NTSC-U/C: 0x0046e2d8, PAL: 0x004abe48
void TunnelSeeker::Load(Stream &stream) {
    ReadObjectRef(stream, mTrans);
    stream.ReadLE(&mTargetRing, sizeof(mTargetRing));
    ReadObjectRef(stream, mMesh);
    unsigned char chSavedFlag;
    stream.Read(&chSavedFlag, 1);
    mSavedFlag = chSavedFlag != 0;
    stream.ReadLE(&mLane, sizeof(mLane));
    if (g_nTunnelLoadVersion >= kFrameOffsetRevision) {
        stream.ReadLE(&mMeshFrameOffset, sizeof(mMeshFrameOffset))
            .ReadLE(&mTransFrameOffset, sizeof(mTransFrameOffset))
            .ReadLE(&mLookFrameOffset, sizeof(mLookFrameOffset));
    }
    stream.ReadLE(&mStrip.mFirstSlice, sizeof(mStrip.mFirstSlice))
        .ReadLE(&mStrip.mSliceCount, sizeof(mStrip.mSliceCount))
        .ReadLE(&mStrip.mRing, sizeof(mStrip.mRing));
    ReadObjectRef(stream, mStrip.mMat);
    stream.ReadLE(&mStrip.mColor.r, sizeof(mStrip.mColor.r))
        .ReadLE(&mStrip.mColor.g, sizeof(mStrip.mColor.g))
        .ReadLE(&mStrip.mColor.b, sizeof(mStrip.mColor.b))
        .ReadLE(&mStrip.mColor.a, sizeof(mStrip.mColor.a));
}

} // namespace Rnd
