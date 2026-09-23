#include "app/tnlactivator.h"

#include <cmath>
#include <cstring>

#include "app/application.h"
#include "app/apptunnel.h"
#include "app/tnlplayer.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "game/grooveworld.h"
#include "math/vector3.h"
#include "os/formatstring.h"
#include "rnd/cam.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/particlesys.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"
#include "rnd/tunnelseeker.h"
#include "rnd/view.h"

namespace {

enum IntroState { kIntroPlaying = 0, kIntroEnding = 1, kIntroDone = 2 };

enum XfmRow { kBasisXRow = 0, kBasisYRow = 1, kBasisZRow = 2 };

// Instrument kinds whose track shows the catcher.
enum CatcherKind { kCatcherKindA = 2, kCatcherKindB = 5 };

constexpr float kPi = 3.1415925f;
constexpr float kTwoPi = 6.283185f;
constexpr float kTurnStep = 0.15f;
constexpr float kTurnPerTrack = 0.125f;

constexpr float kTransOffsetSolo = -500.0f;
constexpr float kTransOffsetShared = -550.0f;
constexpr unsigned kSharedPlayerCount = 2;

// Mapping of the seeker offset ramp.
constexpr float kOffsetFrom = 0.0f;
constexpr float kOffsetTo = -225.0f;
constexpr float kOffsetDuration = 480.0f;
constexpr float kHalf = 0.5f;
constexpr float kLevelsPerRampUnit = 3.0f;

constexpr int kBlinkPeriod = 240;
constexpr int kBlinkDimStart = 121;
constexpr float kDimAlpha = 0.5f;

constexpr float kIntroEndFrame = -1920.0f;

template <typename T>
T *FindObject(const char *pszName) {
    return dynamic_cast<T *>(Rnd::g_manager.Find(HxStr(pszName)));
}

} // namespace

// 0x0043b3d8
TnlActivator::TnlActivator(int nIndex, HxStr colorName, TnlPlayer *pOwner)
    : mIndex(nIndex), mRotView(FindObject<Rnd::View>(FormatString("activator rot%d", nIndex))),
      mMesh(FindObject<Rnd::Mesh>(FormatString("activator%d", nIndex))),
      mIntroView(FindObject<Rnd::View>(FormatString("act_%c intro.view", colorName[0]))),
      mFxView(FindObject<Rnd::View>(FormatString("activator fx%d", nIndex))), mOwner(pOwner),
      mPointer(colorName), mCatcher(colorName), mIntroState(kIntroPlaying), mTurning(0),
      mTargetAngle(0.0f), mAngle(0.0f), mTrack(0), mLevel(0), mBlink(0), mGhost(0), mSuppressed(0),
      mGhostView(nullptr), mLeader(FindObject<Rnd::ParticleSys>("leader.ps")) {
    SetRotShowing(1);
    mTransOffset = kTransOffsetSolo;
    if (kSharedPlayerCount <= Application::shared()->GetWorld()->mLocalPlayers.size()) {
        mTransOffset = kTransOffsetShared;
    }
    if (mOwner->mCam) {
        mOwner->mLocalView->AddDraw(mRotView, nullptr);
    }
    mMesh->SetShowing(0);
    mGhostView = FindObject<Rnd::View>(FormatString("ghost%d.view", mOwner->mPlayerNum));
    mGhostView->ClearDraws();
    GetCachedTunnelObject()->GetSeeker(mIndex)->SetMesh(mMesh);
    mFxView->ClearTransList();
    mFxView->ClearDraws();
    mCatcher.AttachTo(mFxView);
    mPointer.AttachTo(mFxView);
    GetCachedTunnelObject()->GetSeeker(mIndex)->SetMeshFrameOffset(0.0f);
    mOffsetRamp.SetRange(kOffsetFrom, kOffsetTo, kOffsetDuration);
    mOffsetRamp.Jump(0.0f);
    SetLeader(0);
}

// 0x0043baf8
void TnlActivator::Update(float flFrame, float flScaledFrame) {
    int nSettled = 1;
    if (mTargetAngle != mAngle) {
        nSettled = 0;
        float flDelta = std::fmod(mTargetAngle - mAngle + kPi, kTwoPi);
        if (flDelta < 0.0f) {
            flDelta += kTwoPi;
        }
        flDelta -= kPi;
        const float flStep = (0.0f < flDelta) ? kTurnStep : -kTurnStep;
        if (std::fabs(flDelta) < std::fabs(flStep)) {
            mAngle = mTargetAngle;
        } else {
            mAngle += flStep;
        }
        const float flCos = std::cos(mAngle);
        const float flSin = std::sin(mAngle);
        const Vector3 basisX{flCos, 0.0f, -flSin, 1.0f};
        const Vector3 basisY{0.0f, 1.0f, 0.0f, 1.0f};
        const Vector3 basisZ{flSin, 0.0f, flCos, 1.0f};
        Rnd::Transformable *pTrans = mRotView;
        std::memcpy(pTrans->mLocalXfm[kBasisXRow], &basisX, sizeof(basisX));
        std::memcpy(pTrans->mLocalXfm[kBasisYRow], &basisY, sizeof(basisY));
        std::memcpy(pTrans->mLocalXfm[kBasisZRow], &basisZ, sizeof(basisZ));
        pTrans->mDirty = 1;
    }
    if (mOffsetRamp.Update(flFrame)) {
        nSettled = 0;
        GetCachedTunnelObject()->GetSeeker(mIndex)->SetMeshFrameOffset(mOffsetRamp.Value());
        GetCachedTunnelObject()->GetSeeker(mIndex)->SetTransFrameOffset(
            mTransOffset + mOffsetRamp.Value() * kHalf);
        GetCachedTunnelObject()->GetSeeker(mIndex)->SetLookFrameOffset(mOffsetRamp.Value() * kHalf);
    }
    if (mTurning && nSettled) {
        mTurning = 0;
        if (!mLevel && mGhost && !mSuppressed) {
            mOwner->mTunnel->ShowTrackGhost(mTrack, mGhostView);
        }
    }
    if (!g_nAppTunnelDisplayMode) {
        int nDim = 0;
        if (mBlink) {
            nDim = kBlinkDimStart <= static_cast<int>(flFrame) % kBlinkPeriod;
        }
        const float flAlpha = nDim ? kDimAlpha : 1.0f;
        mPointer.SetAlpha(flAlpha);
        mCatcher.SetAlpha(flAlpha);
    }
    if (mIntroState == kIntroPlaying) {
        mIntroView->SetFrame(flFrame);
        mIntroView->SetShowing(1);
        if (kIntroEndFrame < flFrame) {
            mMesh->SetShowing(1);
            if (mOwner->mCam) {
                mOwner->mLocalView->RemoveDraw(mRotView);
            }
            mIntroState = kIntroEnding;
        }
    } else if (mIntroState == kIntroEnding) {
        mIntroView->SetFrame(flFrame);
        if (0.0f < flFrame) {
            mIntroView->SetShowing(0);
            mIntroState = kIntroDone;
        }
    }
    mCatcher.Update(flScaledFrame);
    mPointer.Update(flScaledFrame);
}

// 0x00455ac8
void TnlActivator::SetRotShowing(int nShowing) {
    mRotView->SetShowing(nShowing);
}

// 0x00455a40
void TnlActivator::SetLeader(int nLeader) {
    if (nLeader) {
        mFxView->AddTrans(mLeader);
    } else {
        mFxView->RemoveTrans(mLeader);
    }
    mLeader->SetShowing(nLeader);
}

// 0x00455c98
void TnlActivator::SetGhost(int nGhost) {
    mGhost = nGhost;
    if (mLevel) {
        return;
    }
    if (nGhost) {
        mOwner->mTunnel->ShowTrackGhost(mTrack, mGhostView);
    } else {
        mOwner->mTunnel->HideTrackGhost(mTrack);
    }
}

// 0x00455af8
void TnlActivator::SetSuppressed(int nSuppressed) {
    SetRotShowing(nSuppressed ^ 1); // Yes, the binary flips the low bit rather than testing zero.
    const int nGhost = mGhost;
    mSuppressed = nSuppressed;
    SetGhost(nSuppressed ? 0 : (nGhost != 0));
    mGhost = nGhost;
}

// 0x00455b70
void TnlActivator::MoveToTrack(int nLevel, int nKind, float flTrack) {
    if (!mTurning && !mLevel && mGhost && !mSuppressed) {
        mOwner->mTunnel->HideTrackGhost(mTrack);
    }
    mTrack = static_cast<int>(flTrack);
    mLevel = nLevel;
    mTurning = 1;
    mTargetAngle = (1.0f - flTrack * kTurnPerTrack) * kTwoPi;
    mOffsetRamp.SetTarget(static_cast<float>(nLevel) / kLevelsPerRampUnit);
    mPointer.SetKind(nKind);
    mCatcher.mView->SetShowing((nKind == kCatcherKindA || nKind == kCatcherKindB) ? 1 : 0);
}
