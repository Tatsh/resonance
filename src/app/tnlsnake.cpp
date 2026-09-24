#include "app/tnlsnake.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "app/tnlname.h"
#include "app/tunnelcache.h"
#include "math/transform.h"
#include "math/vector3.h"
#include "os/hxstr.h"
#include "os/r250.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/mesh.h"
#include "rnd/string.h"
#include "rnd/transformable.h"
#include "rnd/tunnel.h"
#include "rnd/view.h"

namespace {

// Start frame of an idle snake, far beyond a song's length.
constexpr float kNoFrame = 1e9f;

constexpr int kPointCount = 16;
constexpr float kRibbonWidth = 0.2f;

// Song frames from the start to the end of a run.
constexpr float kRunFrames = 11520.0f;

// The head moves this many frames per song frame, and each point trails the next by kPointSpacing.
constexpr float kHeadSpeed = 4.0f;
constexpr int kPointSpacing = 100;

constexpr float kGlowDecay = 0.15f;

// Chance per update of a new pulse, and its shape across four neighbouring points.
constexpr double kPulseChance = 0.2;
constexpr int kPulseWidth = 3;
constexpr float kPulseEdge = 0.5f;
constexpr float kPulseCentre = 1.0f;

// One unit in the last place below the float nearest pi, as the binary stores it.
constexpr float kPi = 3.1415925f;

// Song frames per half period of the swing.
constexpr float kSwingHalfPeriod = 960.0f;

// The ring blend is `sin * kBlendScale + kBlendScale`, and the tangent scale is
// `mAmplitude * kTangentSwing * cos + kTangentBase`. Both sums are formed in double precision.
constexpr double kBlendScale = 0.5;
constexpr float kTangentSwing = 0.2f;
constexpr double kTangentBase = 0.75;

// Row of a transform that stores the translation.
constexpr int kXfmRowTranslation = 3;

} // namespace

// 0x0043e6a0
TnlSnake::TnlSnake()
    : mStartFrame(kNoFrame), mString(Rnd::String::NewString(NextAppTunnelName())),
      mHead(Rnd::NewMeshThroughHook(NextAppTunnelName())),
      mView(dynamic_cast<Rnd::View *>(Rnd::g_manager.Find(HxStr("tnl transparent")))) {
    // The binary releases the name before SetMat(), so the lookup is its own statement.
    Rnd::Mat *pMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("snake.mat")));
    mString->SetMat(pMat);
    mString->SetNumPoints(kPointCount);
    mString->SetWidth(kRibbonWidth);
    mString->SetShowing(0);
    mView->AddDraw(mString, nullptr);

    mGlow.resize(mString->GetNumPoints(), 0.0f);
    mPointFrames.resize(mString->GetNumPoints(), 0);

    Rnd::Mesh *pTemplate = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr("snake head.mesh")));
    mHead->Copy(pTemplate, 0);
    mString->AddDraw(mHead, nullptr);
}

// 0x0043ed50
TnlSnake::~TnlSnake() {
    mView->RemoveDraw(mString);
    mString->RemoveDraw(mHead);
    delete mString;
    delete mHead;
}

// 0x00456960
void TnlSnake::Start(
    float flFrame, int nRing, const Color &color, float flPhase, float flAmplitude) {
    mString->SetShowing(1);
    mRing = nRing;
    mStartFrame = flFrame;
    mEndFrame = flFrame + kRunFrames;
    mColor = color;
    mPhase = flPhase;
    mAmplitude = flAmplitude;
    const int nPoints = mString->GetNumPoints();
    for (int i = 0; i < nPoints; ++i) {
        mString->SetPointColor(i, mColor);
    }
}

// 0x0043eec0
void TnlSnake::Update(float flFrame) {
    if (mStartFrame == kNoFrame || mLastFrame == flFrame) {
        return;
    }

    const int nHeadFrame = static_cast<int>((flFrame - mStartFrame) * kHeadSpeed + mStartFrame);
    mLastFrame = flFrame;
    if (mEndFrame < static_cast<float>(nHeadFrame)) {
        mStartFrame = kNoFrame;
        mString->SetShowing(0);
    }

    const int nPoints = mString->GetNumPoints();
    int nFrame = (nHeadFrame / kPointSpacing - (nPoints - 1)) * kPointSpacing;
    for (int i = nPoints - 1; i >= 0; --i, nFrame += kPointSpacing) {
        SetPointFrame(nFrame, i);
        if (mGlow[i] < 0.0f) {
            mGlow[i] = 0.0f;
        }
        const Color color{std::min(mGlow[i] + mColor.r, 1.0f),
                          std::min(mGlow[i] + mColor.g, 1.0f),
                          std::min(mGlow[i] + mColor.b, 1.0f),
                          1.0f};
        mString->SetPointColor(i, color);
        mGlow[i] -= kGlowDecay;
        if (mGlow[i] < 0.0f) {
            mGlow[i] = 0.0f;
        }
    }

    Rnd::Transformable &trans = *mHead;
    std::memcpy(trans.mLocalXfm[kXfmRowTranslation],
                mString->GetPointPos(0),
                sizeof(trans.mLocalXfm[kXfmRowTranslation]));
    trans.mDirty = 1;
    mHead->UpdateWorldXfm(nullptr, 0);

    if (RandomFloat() < kPulseChance) {
        const int nPulse = RandomInt(0, nPoints - kPulseWidth);
        mGlow[nPulse] += kPulseEdge;
        mGlow[nPulse + 1] += kPulseCentre;
        mGlow[nPulse + 2] += kPulseCentre;
        mGlow[nPulse + 3] += kPulseEdge;
    }
}

// 0x0043f218
void TnlSnake::SetPointFrame(int nFrame, int nIndex) {
    if (mPointFrames[nIndex] == nFrame) {
        return;
    }
    for (std::vector<int>::iterator it = mPointFrames.begin(); it != mPointFrames.end(); ++it) {
        if (*it == nFrame) {
            mString->SetPointPos(
                nIndex, *mString->GetPointPos(static_cast<int>(it - mPointFrames.begin())));
            mPointFrames[nIndex] = nFrame;
            return;
        }
    }

    const float flFrame = static_cast<float>(nFrame);
    const float flSwing = flFrame * kPi / kSwingHalfPeriod;
    const float flBlend = static_cast<float>(sinf(mPhase + flSwing) * kBlendScale + kBlendScale);
    const float flTangent =
        static_cast<float>(mAmplitude * kTangentSwing * cosf(mPhase + flSwing) + kTangentBase);
    Transform xfm;
    GetCachedTunnelObject()->ProjectSectionToCameraSpace(mRing, &xfm, flFrame, flBlend, flTangent);
    mString->SetPointPos(nIndex, xfm.mTranslation);
    mPointFrames[nIndex] = nFrame;
}
