#include "app/hudscreenflash.h"

#include "math/color.h"
#include "os/cycles.h"
#include "os/hxstr.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"

namespace {

// Start time that marks no fade in progress.
constexpr float kNoFade = 1.0e9f;

// Shortest duration a fade may take.
constexpr float kShortestFade = 1.0f;

// Numerator of mScale, the milliseconds in one second.
constexpr float kScaleNumerator = 1000.0f;

} // namespace

// NTSC-U/C: 0x0041b150, PAL: 0x00455a88
HudScreenFlash::HudScreenFlash() : mStart(kNoFade), mRate(1.0f) {
    mMesh = dynamic_cast<Rnd::Mesh *>(Rnd::TheManager.Find(HxStr("HUD screen rect")));
    mMesh->SetShowing(1);
    mMesh->SetVertexColor(Color{0.0f, 0.0f, 0.0f, 1.0f});
    mScale =
        kScaleNumerator / static_cast<float>(static_cast<long long>(GetMillisecondsPerSecond()));
}

// NTSC-U/C: 0x0042a548, PAL: 0x00465898
void HudScreenFlash::Start(float flDuration, int nFadeIn) {
    // The binary converts through the 64-bit integer to float routine.
    mStart = static_cast<float>(static_cast<long long>(GetElapsedMilliseconds())) * mScale;
    if (flDuration < kShortestFade) {
        flDuration = kShortestFade;
    }
    mFadeIn = nFadeIn;
    mRate = 1.0f / flDuration;
}

// NTSC-U/C: 0x0041b2a0, PAL: 0x00455bf8
void HudScreenFlash::SetFrame() {
    const float flNow =
        static_cast<float>(static_cast<long long>(GetElapsedMilliseconds())) * mScale;
    if (flNow < mStart) {
        return;
    }

    float flAlpha = (flNow - mStart) * mRate;
    if (1.0f < flAlpha) {
        mStart = kNoFade;
        flAlpha = 1.0f;
    }
    if (mFadeIn != 0) {
        flAlpha = 1.0f - flAlpha;
    }
    mMesh->SetVertexColor(Color{0.0f, 0.0f, 0.0f, flAlpha});
    mMesh->SetShowing(1);
}
