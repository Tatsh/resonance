#include "app/tnlbumpfx.h"

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
