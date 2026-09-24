#include "app/tnlplayer.h"

#include <cstring>

#include "app/application.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "game/grooveworld.h"
#include "game/player.h"
#include "math/transform.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/cam.h"
#include "rnd/manager.h"
#include "rnd/transanim.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"
#include "rnd/tunnelseeker.h"
#include "rnd/view.h"

namespace {

// Player::Slot2() of a player without a local slot.
constexpr int kNoLocalSlot = -1;

constexpr float kUnsetFrame = 1e9f;

// Length of the crippler camera and activator paths, in frames.
constexpr float kCrippleFrames = 5000.0f;

template <typename T>
T *FindObject(const char *pszName) {
    return dynamic_cast<T *>(Rnd::g_manager.Find(HxStr(pszName)));
}

// Copy identity into the local transform of pTrans and mark it dirty.
inline void ResetLocalXfm(Rnd::Transformable *pTrans, const Transform &identity) {
    std::memcpy(pTrans->mLocalXfm, &identity, sizeof(pTrans->mLocalXfm));
    pTrans->mDirty = 1;
}

} // namespace

// 0x00440020
TnlPlayer::TnlPlayer(Player *pPlayer, int nIndex, AppTunnel *pTunnel)
    : mPlayerNum(pPlayer->Slot2() + 1), mCrippleFrame(kUnsetFrame),
      mCrippleActPath(FindObject<Rnd::TransAnim>("crip act path")), mActivatorFx(nullptr),
      mCrippleCamPath(FindObject<Rnd::TransAnim>("crip cam path")), mCamFx(nullptr),
      mCam(FindObject<Rnd::Cam>(FormatString("tnl cam%d", mPlayerNum))), mCamIntro(nullptr),
      mLocalView(FindObject<Rnd::View>(FormatString("tnl local%d.view", mPlayerNum))),
      mTunnel(pTunnel), mUnknown28(0), mIndex(nIndex), mPlayer(pPlayer),
      mActivator(nIndex, pPlayer->mColorName, this), mGridMarkers(pTunnel, mPlayerNum),
      mSabreTrail(nIndex, pPlayer->mColorName),
      mSeekerFade(nIndex, TnlColorFromName(HxStr(pPlayer->mColorName))) {
    Rnd::TunnelSeeker *pSeeker = GetCachedTunnelObject()->GetSeeker(nIndex);
    const Transform identity{
        {1.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f},
        {0.0f, 0.0f, 0.0f, 1.0f},
    };
    // The binary does not test either effect transform for null.
    mActivatorFx = FindObject<Rnd::Transformable>(FormatString("activator fx%d", mIndex));
    ResetLocalXfm(mActivatorFx, identity);
    if (pPlayer->Slot2() != kNoLocalSlot) {
        pSeeker->SetTrans(
            FindObject<Rnd::Transformable>(FormatString("tnl cam slide%d", mPlayerNum)));
        mCamFx = FindObject<Rnd::Transformable>(FormatString("tnl cam fx%d", mPlayerNum));
        ResetLocalXfm(mCamFx, identity);
        mCamIntro = FindObject<Rnd::TransAnim>(FormatString(
            "tnl cam intro%d.tnm",
            static_cast<int>(Application::shared()->GetWorld()->mLocalPlayers.size())));
    } else {
        pSeeker->SetTrans(nullptr);
        mCamFx = nullptr;
        mCam = nullptr;
        mCamIntro = nullptr;
    }
    if (Application::shared()->IsJukeboxMode()) {
        mCamIntro = nullptr;
    }
}

// 0x00440b48
void TnlPlayer::Update(float flFrame, float flScaledFrame) {
    mActivator.Update(flFrame, flScaledFrame);
    mGridMarkers.Update(flFrame);
    mSabreTrail.Update(flFrame);
    mSeekerFade.Update();
    if (mCamIntro && flFrame <= 0.0f) {
        mCamIntro->SetTrans(mCam);
        mCamIntro->SetFrame(flFrame);
    }
    if (mCrippleFrame != kUnsetFrame) {
        const float flElapsed = flFrame - mCrippleFrame;
        if (mCamFx) {
            mCrippleCamPath->SetTrans(mCamFx);
            mCrippleCamPath->SetFrame(flElapsed);
        }
        mCrippleActPath->SetTrans(mActivatorFx);
        mCrippleActPath->SetFrame(flElapsed);
        if (kCrippleFrames < flElapsed) {
            mCrippleFrame = kUnsetFrame;
        }
    }
}
