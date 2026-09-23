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
    mUnknown08->SetShowing(0);
    mUnknown1c = 0;
}

void HudTextMessage::SetFrame(float flTime) {
    if (mUnknown10 == kMessageIdle) {
        return;
    }

    if (mUnknown10 == kMessageStartPending) {
        mUnknown10 = flTime;
    }

    const float flElapsed = flTime - mUnknown10;
    if (flElapsed < kFadeLength) {
        mUnknown0c->SetFrame(flElapsed);
    } else if (flElapsed < mUnknown18) {
        mUnknown0c->SetFrame(kFadeLength);
    } else {
        mUnknown0c->SetFrame(flElapsed - mUnknown18 + kFadeLength);
    }

    if (flElapsed - mUnknown18 > kFadeLength) {
        mUnknown1c = 0;
        mUnknown10 = kMessageIdle;
        mUnknown08->SetShowing(0);
    }
}
