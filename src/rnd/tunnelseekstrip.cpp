#include "rnd/tunnelseekstrip.h"

#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/tunnel.h"

namespace Rnd {

namespace {

// The section ring buffer is always this long.
constexpr unsigned kSectionCount = 3;
constexpr char kSectionNameFormat[] = "[%s_seek%d.%d]";

} // namespace

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

// 0x0046e830
void TunnelSeekStrip::Build(Tunnel *pTunnel, TunnelSeeker *pOwner, int nIndex) {
    mTunnel = pTunnel;
    mOwner = pOwner;
    mSections.resize(kSectionCount, TunnelSeekSection());
    const HxStr &tunnelName = pTunnel->mName;
    for (unsigned i = 0; i < mSections.size(); ++i) {
        const char *pszTunnelName = tunnelName.mStr != nullptr ? tunnelName.mStr : g_szEmptyString;
        mSections[i].Build(HxStr(FormatString(kSectionNameFormat, pszTunnelName, nIndex, i)),
                           pTunnel->mUnknowna4.front());
        mSections[i].mMeshes.front()->SetMaterialChain(mMat);
    }
    mStepNumerator = static_cast<int>(pTunnel->mUnknown9c);
    mStepCount = pTunnel->mUnknowna0;
    mStepSize = mStepNumerator / mStepCount;
    if (mMat != nullptr) {
        mMat->AddRef(mTunnel);
    }
    Refresh();
}

// 0x00477f18
void TunnelSeekStrip::SetMat(Mat *pMat) {
    if (mMat != nullptr) {
        mMat->RemoveRef(mTunnel);
    }
    mMat = pMat;
    if (pMat != nullptr) {
        pMat->AddRef(mTunnel);
    }
    for (TunnelSeekSection &section : mSections) {
        section.mMeshes.front()->SetMaterialChain(pMat);
    }
}

// 0x00477e68
void TunnelSeekStrip::SetColor(const Color &color) {
    mColor = color;
    for (TunnelSeekSection &section : mSections) {
        if (!section.mDirty) {
            section.mMeshes.front()->SetVertexColor(color);
        }
    }
}

// 0x0046ec40
void TunnelSeekStrip::DrawSection(int nSlice, float flScreenSize) {
    TunnelSeekSection &section = mSections[static_cast<unsigned>(nSlice) % mSections.size()];
    if (section.mSlice != nSlice) {
        return;
    }
    if (section.mDirty) {
        // The image also tests mTunnel->mSliceCount for zero with a divide trap here, and uses no
        // quotient.
        section.Update(mTunnel, mColor);
    }
    section.mMeshes.Draw(flScreenSize);
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
