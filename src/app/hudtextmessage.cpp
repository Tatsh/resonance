#include "app/hudtextmessage.h"

#include "rnd/text.h"
#include "rnd/transanim.h"

namespace {

// Start time that marks the message idle.
constexpr float kMessageIdle = 0.0f;

// Start time that marks a start not yet recorded.
constexpr float kMessageStartPending = 1.0e9f;

// Length of each fade, and the animation frame the fade-in ends on.
constexpr float kFadeLength = 250.0f;

} // namespace

void HudTextMessage::Hide() {
    mText->SetShowing(0);
    mActive = 0;
}

void HudTextMessage::SetFrame(float flTime) {
    if (mStart == kMessageIdle) {
        return;
    }

    if (mStart == kMessageStartPending) {
        mStart = flTime;
    }

    const float flElapsed = flTime - mStart;
    if (flElapsed < kFadeLength) {
        mAnim->SetFrame(flElapsed);
    } else if (flElapsed < mHold) {
        mAnim->SetFrame(kFadeLength);
    } else {
        mAnim->SetFrame(flElapsed - mHold + kFadeLength);
    }

    if (flElapsed - mHold > kFadeLength) {
        mActive = 0;
        mStart = kMessageIdle;
        mText->SetShowing(0);
    }
}
