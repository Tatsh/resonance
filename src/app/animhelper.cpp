#include "app/animhelper.h"

#include "rnd/animatable.h"

namespace {

// Start time that marks the range idle.
constexpr float kIdle = -1.0f;

// Start time that marks a run not yet started.
constexpr float kNotStarted = 0.0f;

} // namespace

// NTSC-U/C: 0x00411a18, PAL: 0x0044b4e0
AnimHelper::AnimHelper() : mStart(kIdle), mAnim(nullptr) {
}

// NTSC-U/C: 0x00411a30, PAL: 0x0044b4f8
void AnimHelper::SetAnim(Rnd::Animatable *pAnim) {
    mAnim = pAnim;
    mAnim->SetFrame(0.0f);
}

// NTSC-U/C: 0x00411a58, PAL: 0x0044b520
void AnimHelper::Play(float flFrom, float flTo) {
    mFrom = mAnim->InverseFilters(flFrom);
    mTo = mAnim->InverseFilters(flTo);
    mStart = kNotStarted;
}

// NTSC-U/C: 0x00411ab8, PAL: 0x0044b580
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
