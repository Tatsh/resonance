#include "app/animrange.h"

#include "rnd/animatable.h"

namespace {

// Start time that marks the range idle.
constexpr float kIdle = -1.0f;

// Start time that marks a run not yet started.
constexpr float kNotStarted = 0.0f;

} // namespace

AnimRange::AnimRange() : mStart(kIdle), mAnim(nullptr) {
}

void AnimRange::SetAnim(Rnd::Animatable *pAnim) {
    mAnim = pAnim;
    mAnim->SetFrame(0.0f);
}

void AnimRange::Play(float flFrom, float flTo) {
    mFrom = mAnim->InverseFilters(flFrom);
    mTo = mAnim->InverseFilters(flTo);
    mStart = kNotStarted;
}

int AnimRange::Update(float flTime) {
    if (mStart == kIdle) {
        return 0;
    }

    if (mStart == kNotStarted) {
        mStart = flTime;
    }

    const float flFrame = mFrom + (flTime - mStart);
    if (mTo <= flFrame) {
        mStart = kIdle;
        mAnim->SetFrame(mTo);
        return 0;
    }

    mAnim->SetFrame(flFrame);
    return 1;
}
