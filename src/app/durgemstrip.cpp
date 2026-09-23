#include "app/durgemstrip.h"

#include "app/durgemtrails.h"
#include "app/tunnelcache.h"
#include "math/color.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "rnd/string.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

// Tangent scale every lane position is taken with.
constexpr float kTangentScale = 0.96f;

// Longest trail, in tunnel frames, and how far the playhead passes a stopped head before the strip
// is freed.
constexpr float kMaxLengthFrames = 720.0f;
constexpr float kLingerFrames = 480.0f;

// mId of a strip that draws no trail.
constexpr int kFreeId = -1;

// Ribbon point indices.
enum { kTailPoint = 0, kHeadPoint = 1, kPointCount = 2 };

inline void SetPaddingWords(Transform &xfm) {
    xfm.mBasisX.w = 1.0f;
    xfm.mBasisY.w = 1.0f;
    xfm.mBasisZ.w = 1.0f;
    xfm.mTranslation.w = 1.0f;
}

} // namespace

// 0x004329e8
DurGemStrip::DurGemStrip(Rnd::View *pView) : mId(kFreeId) {
    mString = Rnd::String::NewString(DurGemTrails::NewStringName());
    mString->SetLinePairs(0);
    mString->SetNumPoints(kPointCount);
    mString->SetShowing(0);
    pView->AddDraw(mString);
}

// 0x004372d8
DurGemStrip::~DurGemStrip() {
    delete mString;
}

// 0x00437340
bool DurGemStrip::Start(int nLane,
                        const Color &color,
                        int nId,
                        float flFrame,
                        float flBlend,
                        float flWidth,
                        Rnd::Mat *pMat) {
    if (mId != kFreeId) {
        return false;
    }
    mString->SetWidth(flWidth);
    mString->SetMat(pMat);
    mString->SetShowing(1);
    mId = nId;
    mStopped = 0;
    mLane = nLane;
    mBlend = flBlend;
    mEnd = flFrame;
    mStart = flFrame;
    mString->SetPointColor(kTailPoint, color);
    mString->SetPointColor(kHeadPoint, color);

    Transform xfm;
    SetPaddingWords(xfm);
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(mLane, &xfm, mEnd, mBlend, kTangentScale);
    mString->SetPointPos(kTailPoint, xfm.mTranslation);
    mString->SetPointPos(kHeadPoint, xfm.mTranslation);
    return true;
}

// 0x00432b90
void DurGemStrip::Update(float flFrame) {
    if (mId == kFreeId) {
        return;
    }
    if (mEnd < flFrame && mStopped == 0) {
        mEnd = flFrame;
        Transform xfm;
        SetPaddingWords(xfm);
        GetCachedTunnelObject()->ProjectSectionToCameraSpace(
            mLane, &xfm, mEnd, mBlend, kTangentScale);
        mString->SetPointPos(kHeadPoint, xfm.mTranslation);
        if (mEnd - mStart > kMaxLengthFrames) {
            mStart = mEnd - kMaxLengthFrames;
            GetCachedTunnelObject()->ProjectSectionToCameraSpace(
                mLane, &xfm, mStart, mBlend, kTangentScale);
            mString->SetPointPos(kTailPoint, xfm.mTranslation);
        }
    }
    if (flFrame - mEnd > kLingerFrames) {
        mString->SetShowing(0);
        mId = kFreeId;
    }
}

// 0x00437480
bool DurGemStrip::Stop(int nId, float flFrame) {
    if (mId != nId) {
        return false;
    }
    Update(flFrame);
    mStopped = 1;
    return true;
}

// 0x004374c0
bool DurGemStrip::GetHeadPos(float flFrame, Vector3 *pOut) {
    if (mId == kFreeId) {
        return false;
    }
    const bool bOnTrail = mStart <= flFrame && flFrame <= mEnd;
    if (bOnTrail) {
        Transform xfm;
        SetPaddingWords(xfm);
        GetCachedTunnelObject()->ProjectSectionToCameraSpace(
            mLane, &xfm, flFrame, mBlend, kTangentScale);
        *pOut = xfm.mTranslation;
    }
    return bOnTrail;
}
