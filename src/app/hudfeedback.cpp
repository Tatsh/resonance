#include "app/hudfeedback.h"

#include "gfx/gfxdevice.h"

namespace {

// Song positions that bound the fading part of the feedback, in MIDI ticks.
constexpr float kFadeStart = -5500.0f;
constexpr float kFadeEnd = -2000.0f;
constexpr float kFadeLength = 3500.0f;

// Blend factor of the feedback at full strength, and its texture inset during and before the fade.
constexpr float kFullAlpha = 0.6f;
constexpr float kFadeInset = 100.0f;
constexpr int kFullInset = 75;

} // namespace

// 0x0041bc50
void HudFeedback::SetFrame(float flFrame) {
    if (0.0f < flFrame) {
        return;
    }

    if (flFrame == mLastFrame) {
        g_gfxDevice.mFeedbackEnabled = 0;
        return;
    }
    mLastFrame = flFrame;
    if (kFadeEnd < flFrame) {
        g_gfxDevice.mFeedbackEnabled = 0;
        return;
    }

    if (kFadeStart < flFrame) {
        const float flRemaining = 1.0f - (flFrame - kFadeStart) / kFadeLength;
        g_gfxDevice.mFeedbackInset = static_cast<int>(flRemaining * kFadeInset);
        g_gfxDevice.mFeedbackAlpha = flRemaining * kFullAlpha;
    } else {
        g_gfxDevice.mFeedbackAlpha = kFullAlpha;
        g_gfxDevice.mFeedbackInset = kFullInset;
    }
    g_gfxDevice.mFeedbackRect = GfxDevice::Rect{0.0f, 0.0f, 1.0f, 1.0f};
    g_gfxDevice.mFeedbackEnabled = 1;
}
