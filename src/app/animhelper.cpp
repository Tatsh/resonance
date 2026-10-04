#include "app/animhelper.h"

#include "rnd/animatable.h"

namespace {

// Start time that marks the range idle.
constexpr float kIdle = -1.0f;

// Start time that marks a run not yet started.
constexpr float kNotStarted = 0.0f;

} // namespace

AnimHelper::AnimHelper() : mStart(kIdle), mAnim(nullptr) {
}

void AnimHelper::SetAnim(Rnd::Animatable *pAnim) {
    mAnim = pAnim;
    mAnim->SetFrame(0.0f);
}

void AnimHelper::Play(float flFrom, float flTo) {
    mFrom = mAnim->UnfilterFrame(flFrom);
    mTo = mAnim->UnfilterFrame(flTo);
    mStart = kNotStarted;
}

int AnimHelper::Update(float flTime) {
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
