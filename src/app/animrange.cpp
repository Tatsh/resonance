#include "app/animrange.h"

#include "rnd/animatable.h"

namespace {

// Start time that marks the range idle.
constexpr float kIdle = -1.0f;

// Start time that marks a run not yet started.
constexpr float kNotStarted = 0.0f;

} // namespace

// 0x00411a18
AnimRange::AnimRange() : mStart(kIdle), mAnim(nullptr) {
}

// 0x00411a30
void AnimRange::SetAnim(Rnd::Animatable *pAnim) {
    mAnim = pAnim;
    mAnim->SetFrame(0.0f);
}

// 0x00411a58
void AnimRange::Play(float flFrom, float flTo) {
    mFrom = mAnim->InverseFilters(flFrom);
    mTo = mAnim->InverseFilters(flTo);
    mStart = kNotStarted;
}

// 0x00411ab8
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
