#include "app/linearanim.h"

namespace {

// Last time that marks no time recorded.
constexpr float kNoTime = 9999999.0f;

// The default mapping the constructor sets.
constexpr float kDefaultTo = 100.0f;
constexpr float kDefaultDuration = 240.0f;

// How far short of the target Jump() places the raw value.
constexpr double kJumpShortfall = 1.0e-6;

} // namespace

// NTSC-U/C: 0x00411878, PAL: 0x0044b340
LinearAnim::LinearAnim() : mLastTime(kNoTime), mTarget(0.0f), mCurrent(0.0f) {
    SetParams(mTarget, kDefaultTo, kDefaultDuration);
}

// NTSC-U/C: 0x004118d0, PAL: 0x0044b398
void LinearAnim::SetParams(float flFrom, float flTo, float flDuration) {
    mScale = flTo - flFrom;
    mRate = 1.0f / flDuration;
    mOffset = flFrom - mScale * 0.0f; // Yes, the binary multiplies by zero here.
}

// NTSC-U/C: 0x00411900, PAL: 0x0044b3c8
void LinearAnim::SetTarget(float flTarget) {
    mTarget = flTarget;
}

// NTSC-U/C: 0x00411908, PAL: 0x0044b3d0
void LinearAnim::Jump(float flTarget) {
    mTarget = flTarget;
    mCurrent = static_cast<float>(flTarget - kJumpShortfall);
}

// NTSC-U/C: 0x00411958, PAL: 0x0044b420
float LinearAnim::Val() const {
    return mCurrent * mScale + mOffset;
}

// NTSC-U/C: 0x00411970, PAL: 0x0044b438
int LinearAnim::Execute(float flTime) {
    if (mCurrent == mTarget) {
        mLastTime = flTime;
        return 0;
    }

    if (mLastTime == kNoTime) {
        mLastTime = flTime;
    }

    const float flDistance = mTarget - mCurrent;
    float flStep = (flTime - mLastTime) * mRate;
    if (!(0.0f < flDistance)) {
        flStep = -flStep;
    }

    if (flDistance * flDistance < flStep * flStep) {
        mCurrent = mTarget;
    } else {
        mCurrent = mCurrent + flStep;
    }
    mLastTime = flTime;
    return 1;
}
