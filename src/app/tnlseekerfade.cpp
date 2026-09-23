#include "app/tnlseekerfade.h"

#include "app/application.h"
#include "app/tunnelcache.h"
#include "game/gamemanagerimpl.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/tunnel.h"
#include "rnd/tunnelseeker.h"

namespace {

// Range fields before the owner stores a range.
constexpr int kNoSlice = -1;
constexpr int kNoRing = -1;

// Alpha added per update while a seeker fades back in.
constexpr float kFadeInRate = 0.2f;

} // namespace

TnlSeekerFade::TnlSeekerFade(int nIndex, const Color &color)
    : mActive(1), mFirstSlice(kNoSlice), mSliceCount(0), mRing(kNoRing), mColor(color),
      mFadeRate(0.0f), mIndex(nIndex) {
    Rnd::Mat *pMat;
    if (Application::shared()->GetPlayMode() == kPlayModeGame) {
        pMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("seeker.mat")));
    } else {
        pMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("seeker_loop.mat")));
    }
    mColor.a = 1.0f;
    GetCachedTunnelObject()->GetSeeker(mIndex)->SetMat(pMat);
    GetCachedTunnelObject()->GetSeeker(mIndex)->SetColor(mColor);
    SetActive(1);
}

void TnlSeekerFade::SetActive(int nActive) {
    mActive = nActive;
    if (nActive) {
        GetCachedTunnelObject()->GetSeeker(mIndex)->SetRange(mFirstSlice, mSliceCount, mRing);
    } else {
        GetCachedTunnelObject()->GetSeeker(mIndex)->SetRange(0, 0, 0);
    }
}

void TnlSeekerFade::Update() {
    float flAlpha = mColor.a + mFadeRate;
    if (flAlpha < 0.0f) {
        if (mActive) {
            GetCachedTunnelObject()->GetSeeker(mIndex)->SetRange(mFirstSlice, mSliceCount, mRing);
        }
        if (mSliceCount == 0) {
            mFadeRate = 0.0f;
        } else {
            mFadeRate = kFadeInRate;
        }
        flAlpha = 0.0f;
    } else if (1.0f < flAlpha) {
        flAlpha = 1.0f;
    }
    if (mColor.a != flAlpha) {
        mColor.a = flAlpha;
        GetCachedTunnelObject()->GetSeeker(mIndex)->SetColor(mColor);
    }
}
