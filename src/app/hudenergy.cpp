#include "app/hudenergy.h"

#include "app/application.h"
#include "game/gamemanagerimpl.h"
#include "math/color.h"
#include "os/hxstr.h"
#include "rnd/animatable.h"
#include "rnd/manager.h"
#include "rnd/mesh.h"
#include "rnd/meshvert.h"

namespace {

// Times the constructor runs SetFrame() to settle the empty bar.
constexpr int kSettleFrameCount = 100;

// Frame the constructor settles the bar at.
constexpr float kSettleFrame = -1.0f;

// Animation frames per unit of level.
constexpr float kFramesPerLevel = 100.0f;

// Level below which the bar hides.
constexpr double kEmptyLevel = 0.01;

// Level below which the bar blinks.
constexpr float kLowLevel = 0.2f;

// Length of one blink cycle, and the part of it the bar shows for, in MIDI ticks.
constexpr int kBlinkPeriod = 240;
constexpr int kBlinkShown = 120;

} // namespace

HudEnergy::HudEnergy([[maybe_unused]] int nIndex) {
    Rnd::Mesh *pFrame = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr("HUD1 energy.mesh")));
    mUnknown04 = dynamic_cast<Rnd::Mesh *>(Rnd::g_manager.Find(HxStr("HUD1 energy bar.mesh")));
    mUnknown00 =
        dynamic_cast<Rnd::Animatable *>(Rnd::g_manager.Find(HxStr("HUD1 energy bar.msnm")));

    bool bShowFrame = false;
    if (Application::shared()->GetPlayMode() == kPlayModeGame) {
        bShowFrame = Application::shared()->GetGameMode() == kGameModeSolo;
    }
    pFrame->SetShowing(bShowFrame);

    mUnknown08 = 0.0f;
    for (int i = 0; i < kSettleFrameCount; ++i) {
        SetFrame(kSettleFrame);
    }
}

void HudEnergy::SetFrame(float flFrame) {
    mUnknown00->SetFrame(mUnknown08 * kFramesPerLevel);

    // Yes, the binary copies the first vertex colour and never reads it.
    [[maybe_unused]] const Color vertexColor = mUnknown04->mVertsOwner->mVerts[0].mColor;

    if (mUnknown08 < kEmptyLevel) {
        mUnknown04->SetShowing(0);
    } else if (mUnknown08 < kLowLevel) {
        mUnknown04->SetShowing(static_cast<int>(flFrame) % kBlinkPeriod < kBlinkShown);
    } else {
        mUnknown04->SetShowing(1);
    }
}
