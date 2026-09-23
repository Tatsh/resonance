#include "app/hudpoints.h"

#include "app/overlay.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/blur.h"
#include "rnd/manager.h"
#include "rnd/mat.h"
#include "rnd/text.h"
#include "rnd/view.h"

namespace {

// Frame the constructor parks the exit animation on.
constexpr float kExitRestFrame = 100.0f;

// Multiplier the constructor starts with.
constexpr int kInitialMultiplier = 1;

} // namespace

HudPoints::HudPoints(int nIndex)
    : mFlash(0.0f), mPulse(0.0f), mPulseRest(0.0f), mMultiplier(kInitialMultiplier), mPoints(0),
      mShowing(0), mHot(0), mUnknown38(0) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mExitBlur = dynamic_cast<Rnd::Blur *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pts_exit%d.blur", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mExitView = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pts_exit%d.view", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mExitText = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pts_exit%d.txt", pszLayout, nIndex))));

    mExit.SetAnim(mExitView);
    mExit.Play(kExitRestFrame, kExitRestFrame);

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mPointsText = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pts%d.txt", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mMultiplierText = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s ptsmult%d.txt", pszLayout, nIndex))));

    mPlainMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD ptstmp.mat")));
    mHotMat = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD ptstmphot.mat")));

    mPointsText->SetShowing(0);
    mMultiplierText->SetShowing(0);
}

void HudPoints::SetPoints(int nPoints) {
    mPoints = nPoints;
    mPointsText->SetText(HxStr(FormatString("%d", nPoints)));
    mPointsText->SetShowing(1);
    mFlash = 0.0f;
    mShowing = 1;
    mPulseRest = 0.0f;
}

void HudPoints::SetMultiplier(int nMultiplier) {
    mMultiplier = nMultiplier;
    mMultiplierText->SetText(HxStr(FormatString("x%d", nMultiplier)));
    mMultiplierText->SetShowing(mMultiplier > 1);
}
