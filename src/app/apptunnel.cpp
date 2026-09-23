#include "app/apptunnel.h"

#include <cmath>
#include <cstring>

#include "app/renderer.h"
#include "app/tnlcripplefx.h"
#include "app/tnlfirefx.h"
#include "app/tnlgemmanager.h"
#include "app/tnlnowring.h"
#include "app/tnlpanelfx.h"
#include "app/tnlsnake.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "game/gamemanagerimpl.h"
#include "game/player.h"
#include "math/color.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "rnd/drawable.h"
#include "rnd/mat.h"
#include "rnd/particle.h"
#include "rnd/particlesys.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

// Rate the ghost material's alpha moves at while a ghost shows, negated while it hides.
constexpr float kGhostFadeRate = 0.15f;

// Start frame of a TnlSnake that is not running.
constexpr float kIdleSnakeFrame = 1e9f;

// Push of a string flare placed on a ring, as the tangent scale of the ring transform.
constexpr float kStringFlareRingScale = 0.97f;

// Turn of the now ring per local view, applied negated, and the conversion from degrees.
constexpr float kLocalViewTurnDegrees = 45.0f;
constexpr float kPi = 3.1415925f;
constexpr float kDegreesPerHalfTurn = 180.0f;

} // namespace

AppTunnel *g_pAppTunnel;

int AppTunnel::IsTrackBarLocked(int nTrack, int nBar) {
    // The binary compares the track mode against mPlayMode itself, which is kPlayModeJam here.
    if ((mPlayMode == kPlayModeJam) && (mTrackModes[nTrack] == kTrackModeRiff)) {
        return 0;
    }
    const TrackMode mode = mTrackModes[nTrack];
    if ((mode == kTrackModeAxe) || (mode == kTrackModeScratch) || (mode == kTrackModeVocal)) {
        return 1;
    }
    if (!mRenderer->GetCell(nTrack, nBar)->mPlayer->IsNull()) {
        return 1;
    }
    if (mRenderer->GetCell(nTrack, nBar)->mEnabled) {
        return 0;
    }
    return 1;
}

void AppTunnel::ShowTrackGhost(int nTrack, Rnd::Drawable *pGhost) {
    pGhost->ClearDraws();
    mGemManager->AddKindDraws(mGhostGemKinds[nTrack], pGhost);
    mGemManager->SetKindShowing(mGhostGemKinds[nTrack], 1);
    GetGhostMat(nTrack)->SetAlpha(0.0f);
    mGhostFadeRates[nTrack] = kGhostFadeRate;
}

void AppTunnel::HideTrackGhost(int nTrack) {
    GetGhostMat(nTrack)->SetAlpha(1.0f);
    mGhostFadeRates[nTrack] = -kGhostFadeRate;
}

Rnd::Mat *AppTunnel::GetGhostMat(int nTrack) {
    return mGhostMats[nTrack];
}

void AppTunnel::StartGemFlash(const Vector3 &pos) {
    for (auto it = mGemFlashes.begin(); it != mGemFlashes.end(); ++it) {
        GemFlash *pFlash = *it;
        if (pFlash->mParticle == nullptr) {
            // The binary does not test the new particle for null.
            pFlash->mParticle = pFlash->mSystem->AllocParticle();
            pFlash->mParticle->mCol = Color{1.0f, 1.0f, 1.0f, 1.0f};
            pFlash->mParticle->mSize = 1.0f;
            pFlash->mParticle->mPos = pos;
            return;
        }
    }
}

int AppTunnel::StartPanelFX(int nRing, int nSlice, int nForward) {
    for (auto it = mPanelFX.begin(); it != mPanelFX.end(); ++it) {
        if ((*it)->mState == TnlPanelFX::kStateIdle) {
            (*it)->Start(nRing, nSlice, nForward);
            return 1;
        }
    }
    return 0;
}

void AppTunnel::StartFireFX(float flPathStart,
                            int nIndex,
                            int nSlot,
                            const Color &color,
                            const Color &altColor,
                            float flPathEnd) {
    for (auto it = mFireFX.begin(); it != mFireFX.end(); ++it) {
        if ((*it)->Start(flPathStart, nIndex, nSlot, color, altColor, flPathEnd)) {
            return;
        }
    }
}

int AppTunnel::StartCrippleFX(const std::vector<TnlPlayer *> &targets, float flFrame) {
    for (auto it = mCrippleFX.begin(); it != mCrippleFX.end(); ++it) {
        if ((*it)->mState == TnlCrippleFX::kStateIdle) {
            (*it)->Start(targets, flFrame);
            return 1;
        }
    }
    return 0;
}

int AppTunnel::StartSnake(
    float flFrame, int nRing, const Color &color, float flPhase, float flAmplitude) {
    for (auto it = mSnakes.begin(); it != mSnakes.end(); ++it) {
        if ((*it)->mStartFrame == kIdleSnakeFrame) {
            (*it)->Start(flFrame, nRing, color, flPhase, flAmplitude);
            return 1;
        }
    }
    return 0;
}

void AppTunnel::AddPendingTrigger(TnlTrigger *pTrigger, float flFrame) {
    mPendingTriggers.push_back(TnlPendingTrigger{pTrigger, flFrame});
}

void AppTunnel::PlaceStringFlare(const Vector3 &pos) {
    for (Rnd::Particle *pParticle = mStringFlare->GetLiveParticles(); pParticle != nullptr;
         pParticle = pParticle->mNext) {
        if (pParticle->mCol.a != 1.0f) {
            pParticle->mPos = pos;
            pParticle->mCol.a = 1.0f;
            return;
        }
    }
}

void AppTunnel::PlaceStringFlareOnRing(int nRing, float flBlend) {
    for (Rnd::Particle *pParticle = mStringFlare->GetLiveParticles(); pParticle != nullptr;
         pParticle = pParticle->mNext) {
        if (pParticle->mCol.a != 1.0f) {
            Transform xfm;
            PadTransformRows(xfm);
            GetCachedTunnelObject()->ProjectSectionToCameraSpace(
                nRing, &xfm, mRenderer->mSongTick, flBlend, kStringFlareRingScale);
            pParticle->mPos = xfm.mTranslation;
            pParticle->mCol.a = 1.0f;
            return;
        }
    }
}

void AppTunnel::PrepareLocalView(int nView, [[maybe_unused]] float flFrame) {
    TnlNowRing *pNowRing = mNowRing;
    if (!pNowRing->mResetPending) {
        return;
    }
    const float flAngle =
        ((static_cast<float>(-nView) * kLocalViewTurnDegrees) * kPi) / kDegreesPerHalfTurn;
    const float flCos = std::cos(flAngle);
    const float flSin = std::sin(flAngle);
    const Vector3 basis[] = {
        {flCos, 0.0f, -flSin, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {flSin, 0.0f, flCos, 1.0f},
    };
    std::memcpy(pNowRing->mRotView->mLocalXfm, basis, sizeof(basis));
    pNowRing->mRotView->mDirty = 1;
}
