#include "app/tnlsabretrail.h"

#include <cmath>

#include "app/application.h"
#include "app/tnlutil.h"
#include "app/tunnelcache.h"
#include "game/leveldata.h"
#include "game/trackdata.h"
#include "math/color.h"
#include "math/transform.h"
#include "os/formatstring.h"
#include "rnd/manager.h"
#include "rnd/particle.h"
#include "rnd/particlesys.h"
#include "rnd/string.h"
#include "rnd/tunnel.h"

namespace {

constexpr int kFramesPerBar = 1920;
constexpr float kUnsetFrame = 1e9f;

// Mapping of a gem's lane number onto the 0 through 1 track width.
constexpr float kLaneScale = 0.315f;
constexpr float kLaneOffset = 0.185f;

// Spacing of the filler points between two gems, in frames.
constexpr float kFillerSpacing = 60.0f;

// Scale of the translation row Rnd::Tunnel builds for a point.
constexpr float kTangentScale = 0.96f;

constexpr float kRevealPerFrame = 0.04f;
constexpr float kAmplitudeDecay = 0.02f;
constexpr float kGlowDecay = 0.02f;
constexpr float kGlowParticleScale = 0.4f;
constexpr float kStrengthPerStep = 0.05f;
constexpr float kAmplitudeOverStrength = 0.33f;
constexpr double kGlowStep = 0.1;
constexpr double kAmplitudeScale = 0.4;
constexpr float kFrequencyScale = 0.1f;
constexpr float kFrequencyBase = 0.1f;
constexpr float kWobbleFlip = -0.9f;
constexpr float kTwoPi = 6.283185f;

} // namespace

TnlSabreTrail::TnlSabreTrail(int nIndex, HxStr colorName)
    : mString(nullptr), mIndex(nIndex), mStartFrame(0.0f), mAmplitude(0.0f), mStrength(0.0f),
      mPulseFrame(0.0f), mPhase(0.0f), mGlowFloor(0.0f), mGlowSize(0.0f), mGlow(nullptr) {
    mString = dynamic_cast<Rnd::String *>(
        Rnd::g_manager.Find(HxStr(FormatString("sabre_%c.str", colorName[0]))));
    mGlow = dynamic_cast<Rnd::ParticleSys *>(
        Rnd::g_manager.Find(HxStr(FormatString("gem_glow%d.ps", nIndex))));
    SetShowing(1);
    Clear();
}

TnlSabreTrail::~TnlSabreTrail() {
    Clear();
}

void TnlSabreTrail::SetShowing(int nShowing) {
    mString->SetShowing(nShowing);
}

void TnlSabreTrail::Clear() {
    mGlow->FreeAllParticles();
    mPoints.clear();
    mString->SetNumPoints(0);
    mAmplitude = 0.0f;
    mRevealed = 0;
    mFirstBar = 0;
    mBarCount = 0;
    mTrack = 0;
}

void TnlSabreTrail::Rebuild() {
    const int nBarCount = mBarCount;
    if (nBarCount == 0) {
        return;
    }
    const int nTrack = mTrack;
    const int nFirstBar = mFirstBar;
    Clear();
    Build(nTrack, nFirstBar, nBarCount);
}

void TnlSabreTrail::Build(int nTrack, int nFirstBar, int nBarCount) {
    if (nTrack == mTrack && nFirstBar == mFirstBar && nBarCount == mBarCount && !mPoints.empty()) {
        return;
    }
    Clear();
    mTrack = nTrack;
    mFirstBar = nFirstBar;
    mBarCount = nBarCount;
    mStrength = 0.0f;
    mGlowSize = 0.0f;
    mGlowFloor = 0.0f;
    TrackData *pTrack = Application::shared()->GetLevel()->TrackAt(mTrack);
    for (int nBar = nFirstBar; nBar < nFirstBar + nBarCount; ++nBar) {
        const auto *pGems = pTrack->GetGems(nBar);
        for (auto it = pGems->begin(); it != pGems->end(); ++it) {
            AddSegmentPoint(static_cast<float>(it->mPosition.mTick + nBar * kFramesPerBar),
                            static_cast<float>(it->mValue) * kLaneScale + kLaneOffset);
        }
    }
    mStartFrame = kUnsetFrame;
}

// 0x004389a8
void TnlSabreTrail::AddPoint(float flFrame, float flLane, int nGem) {
    Transform xfm;
    PadTransformRows(xfm);
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(
        mTrack, &xfm, flFrame, flLane, kTangentScale);
    if (nGem) {
        Rnd::Particle *pParticle = mGlow->AllocParticle();
        if (pParticle) {
            pParticle->mPos = xfm.mTranslation;
            pParticle->mCol = Color{1.0f, 1.0f, 1.0f, 1.0f};
            pParticle->mSize = 0.0f;
        }
    }
    Point point;
    point.mPos = xfm.mTranslation;
    point.mFrame = flFrame;
    point.mLane = flLane;
    point.mGem = nGem;
    mPoints.push_back(point);
}

// 0x00454e48
void TnlSabreTrail::AddSegmentPoint(float flFrame, float flLane) {
    if (mPoints.empty()) {
        AddPoint(flFrame, flLane, 1);
        return;
    }
    const Point &last = mPoints.back();
    const float flSlope = (flLane - last.mLane) / (flFrame - last.mFrame);
    const float flIntercept = last.mLane - flSlope * last.mFrame;
    for (float flFiller = last.mFrame + kFillerSpacing; flFiller < flFrame;
         flFiller += kFillerSpacing) {
        AddPoint(flFiller, flSlope * flFiller + flIntercept, 0);
    }
    AddPoint(flFrame, flLane, 1);
}

void TnlSabreTrail::Pulse(float flFrame, int nStrength, [[maybe_unused]] int nTotal) {
    if (mPoints.size() < 2) {
        return;
    }
    if (flFrame < static_cast<float>(mFirstBar * kFramesPerBar)) {
        return;
    }
    mPulseFrame = flFrame;
    // Yes, the binary adds 0.
    float flStrength = static_cast<float>(nStrength) * kStrengthPerStep + 0.0f;
    if (1.0f < flStrength) {
        flStrength = 1.0f;
    } else if (flStrength < 0.0f) {
        flStrength = 0.0f;
    }
    mStrength = flStrength;
    const float flAmplitude = flStrength + kAmplitudeOverStrength;
    mAmplitude = (1.0f < flAmplitude) ? 1.0f : flAmplitude;
    mGlowFloor = flStrength + kGlowStep;
    mGlowSize = mGlowFloor + kGlowStep;
}

void TnlSabreTrail::Update(float flFrame) {
    if (mPoints.size() < 2) {
        return;
    }
    if (mStartFrame == kUnsetFrame) {
        mStartFrame = flFrame;
    }
    if (static_cast<unsigned>(mRevealed) < mPoints.size()) {
        const int nPointCount = static_cast<int>(mPoints.size());
        int nVisible = static_cast<int>((flFrame - mStartFrame) * kRevealPerFrame);
        if (nPointCount < nVisible) {
            nVisible = nPointCount;
        }
        if (mString->GetNumPoints() != nVisible) {
            mString->SetNumPoints(nVisible);
        }
        for (; mRevealed < nVisible; ++mRevealed) {
            mString->SetPointPos(mRevealed, mPoints[mRevealed].mPos);
        }
    }
    Wobble(flFrame);
    mPhase += 1.0f;
    for (Rnd::Particle *pParticle = mGlow->GetLiveParticles(); pParticle;
         pParticle = pParticle->mNext) {
        pParticle->mSize = mGlowSize * kGlowParticleScale;
    }
    const float flShrunk = mGlowSize - kGlowDecay;
    mGlowSize = (flShrunk < mGlowFloor) ? mGlowFloor : flShrunk;
}

// 0x00438e00
void TnlSabreTrail::Wobble(float) {
    if (mAmplitude == 0.0f) {
        return;
    }
    const float flDecayed = mAmplitude - kAmplitudeDecay;
    mAmplitude = (flDecayed < 0.0f) ? 0.0f : flDecayed;
    const float flFrequency = (mStrength * kFrequencyScale + kFrequencyBase) * kTwoPi;
    float flOffset =
        static_cast<float>(mAmplitude * kAmplitudeScale * std::sin(flFrequency * mPhase));
    for (int i = 0; i < mRevealed; ++i) {
        const Point &point = mPoints[i];
        if (mPulseFrame <= point.mFrame && !point.mGem) {
            flOffset *= kWobbleFlip;
            Transform xfm;
            PadTransformRows(xfm);
            GetCachedTunnelObject()->ProjectSectionToCameraSpace(
                mTrack, &xfm, point.mFrame, point.mLane + flOffset, kTangentScale);
            mString->SetPointPos(i, xfm.mTranslation);
        }
    }
}
