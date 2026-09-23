#include "rnd/tunnelseekstrip.h"

#include "rnd/mat.h"
#include "rnd/tunnel.h"

namespace Rnd {

// 0x0046e7d8
TunnelSeekStrip::TunnelSeekStrip()
    : mFirstSlice(0), mSliceCount(0), mRing(0), mColor{1.0f, 1.0f, 1.0f, 1.0f}, mMat(nullptr),
      mTunnel(nullptr), mOwner(nullptr), mStepNumerator(0) {
}

// 0x0046ea98
void TunnelSeekStrip::Clear() {
    mSections.clear();
    if (mMat != nullptr) {
        mMat->RemoveRef(mTunnel);
    }
    mTunnel = nullptr;
}

// 0x0046eb48
void TunnelSeekStrip::SetRange(int nFirstSlice, int nSliceCount, int nRing) {
    mFirstSlice = nFirstSlice;
    mSliceCount = nSliceCount;
    mRing = nRing;
    for (TunnelSeekSection &section : mSections) {
        section.Invalidate();
    }
    if (mSections.size() < static_cast<unsigned>(mSliceCount)) {
        mSliceCount = mSections.size();
    }
    for (int i = 0; i < mSliceCount; ++i) {
        // The binary divides unsigned, so a negative slice selects by its unsigned bit pattern.
        const unsigned nIndex = static_cast<unsigned>(mFirstSlice + i) % mSections.size();
        mSections[nIndex].Set(mFirstSlice + i, mRing, i == mSliceCount - 1, i == 0);
    }
}

// 0x00477ff0
void TunnelSeekStrip::Refresh() {
    SetRange(mFirstSlice, mSliceCount, mRing);
}

// 0x00477db8
void TunnelSeekStrip::Replace(Object *pFrom, Object *pTo, Object *pReferrer) {
    if (mMat == pFrom && mMat != nullptr) {
        mMat->RemoveRef(pReferrer);
        mMat = dynamic_cast<Mat *>(pTo);
        if (mMat != nullptr) {
            mMat->AddRef(pReferrer);
        }
    }
}

} // namespace Rnd
