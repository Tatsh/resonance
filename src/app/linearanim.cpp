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

LinearAnim::LinearAnim() : mLastTime(kNoTime), mTarget(0.0f), mCurrent(0.0f) {
    SetParams(mTarget, kDefaultTo, kDefaultDuration);
}

void LinearAnim::SetParams(float flFrom, float flTo, float flDuration) {
    mScale = flTo - flFrom;
    mRate = 1.0f / flDuration;
    mOffset = flFrom - mScale * 0.0f; // Yes, the binary multiplies by zero here.
}

void LinearAnim::SetTarget(float flTarget) {
    mTarget = flTarget;
}

void LinearAnim::Jump(float flTarget) {
    mTarget = flTarget;
    mCurrent = static_cast<float>(flTarget - kJumpShortfall);
}

float LinearAnim::Val() const {
    return mCurrent * mScale + mOffset;
}

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
