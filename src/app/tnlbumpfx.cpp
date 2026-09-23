#include "app/tnlbumpfx.h"

#include <iterator>
#include <list>

#include "app/tnlutil.h"
#include "math/color.h"
#include "math/quaternion.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/generator.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/transanim.h"

namespace {

// Spawn interval that stops a generator in practice.
constexpr float kSilentRateGen = 1e9f;

constexpr float kPathWindowFrames = 800.0f;
constexpr float kSpawnFrames = 500.0f;
constexpr float kBurstFrames = 1500.0f;

// Turn per lane step, as a fraction of a whole turn, and a whole turn in radians.
constexpr float kStepTurn = 0.125f;
constexpr float kTwoPi = 6.283185f;

} // namespace

TnlBumpFX::TnlBumpFX(int nIndex) : mState(kStateIdle) {
    mGenerator = dynamic_cast<Rnd::Generator *>(
        Rnd::g_manager.Find(HxStr(FormatString("bumpfx%d.mgen", nIndex))));
    mMat =
        dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr(FormatString("bumpfx%d.mat", nIndex))));
    mTransAnim = dynamic_cast<Rnd::TransAnim *>(
        Rnd::g_manager.Find(HxStr(FormatString("bumpfx%d.tnm", nIndex))));

    float flHigh;
    mGenerator->GetRateGen(mSavedRateGen, flHigh);
    mGenerator->SetRateGen(kSilentRateGen, kSilentRateGen);
    mGenerator->SetShowing(0);
}

TnlBumpFX::~TnlBumpFX() {
    mGenerator->SetRateGen(mSavedRateGen, mSavedRateGen);
}

void TnlBumpFX::Start(int nStep, const HxStr &colorName, int nForward, float flPathOffset) {
    mState = kStateSpawning;
    mForward = nForward;
    mPathOffset = flPathOffset;
    mGenerator->SetRateGen(mSavedRateGen, mSavedRateGen);
    mGenerator->mNextSpawnFrame = 0.0f;

    const float angles[] = {
        0.0f,
        (1.0f - static_cast<float>(nStep) * kStepTurn) * kTwoPi,
        0.0f,
        1.0f,
    };
    mTransAnim->GetFramesOwner()->mRotKeys.front().mQuat = EulerAnglesToQuat(angles);

    std::list<Rnd::TransAnim::RotKey> &keys = mTransAnim->GetFramesOwner()->mRotKeys;
    keys.sort();
    if (keys.size() != 1) {
        if (keys.size() == 2) {
            // The binary expands both calls inline here.
            keys.front().ComputeSplineTangents(nullptr, &keys.back());
            keys.back().ComputeSplineTangents(&keys.front(), nullptr);
        } else {
            const auto last = std::prev(keys.end());
            for (auto it = std::next(keys.begin()); it != last; ++it) {
                it->ComputeSplineTangents(&*std::prev(it), &*std::next(it));
            }
            keys.front().ComputeSplineTangents(nullptr, &*std::next(keys.begin()));
            keys.back().ComputeSplineTangents(&*std::prev(last), nullptr);
        }
    }

    mMat->SetEmissive(TnlColorFromName(colorName));
    mGenerator->SetShowing(1);
}

void TnlBumpFX::SetFrame(float flFrame) {
    if (mState == kStateIdle) {
        mStartFrame = flFrame;
        return;
    }

    const float flElapsed = flFrame - mStartFrame;
    if (mState == kStateSpawning) {
        const float flPathStart = flFrame + mPathOffset;
        Rnd::TransAnim *pPath = mGenerator->GetPath();
        if (mForward != 0) {
            mGenerator->SetPath(pPath, flPathStart, flPathStart + kPathWindowFrames);
        } else {
            mGenerator->SetPath(pPath, flPathStart, flPathStart - kPathWindowFrames);
        }
        if (kSpawnFrames < flElapsed) {
            mState = kStateDraining;
            mGenerator->SetRateGen(kSilentRateGen, kSilentRateGen);
        }
    } else if (mState == kStateDraining) {
        if (kBurstFrames < flElapsed) {
            mState = kStateIdle;
            mGenerator->SetShowing(0);
        }
    }
    mGenerator->SetFrame(flElapsed);
}
