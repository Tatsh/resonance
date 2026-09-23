#include "app/hudpanel.h"

#include "app/application.h"
#include "app/overlay.h"
#include "os/hxstr.h"

namespace {

// Frames and time of the two animation ramps.
constexpr float kAssemblyFrom = 0.0f;
constexpr float kAssemblyTo = 100.0f;
constexpr float kLabelSwapFrom = 100.0f;
constexpr float kLabelSwapTo = 200.0f;
constexpr float kRampDuration = 480.0f;

} // namespace

HudPanel::HudPanel()
    : mPosition(Application::shared()->GetPlayMap()), mMessage(HxStr("HUD genmsg.txt")),
      mAssembly(HxStr(g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString) +
                    HxStr(" assembly.view"),
                kAssemblyFrom,
                kAssemblyTo,
                kRampDuration),
      mLabelSwap(HxStr("HUD1 label swap.tnm"), kLabelSwapFrom, kLabelSwapTo, kRampDuration) {
}

void HudPanel::SetFrame(float flFrame, float flTime) {
    mPosition.SetFrame(flFrame);
    mScreenFlash.SetFrame();
    mAssembly.Update(flTime);
    mLabelSwap.Update(flTime);
    mHighlight.SetFrame(flFrame);
    mLetterbox.SetFrame(flFrame);
    mWinMessage.SetFrame(flTime);
    mFeedback.SetFrame(flFrame);
}
