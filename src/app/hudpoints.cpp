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
    : mUnknown00(0.0f), mUnknown04(0.0f), mUnknown08(0.0f), mUnknown0c(kInitialMultiplier),
      mUnknown10(0), mUnknown14(0), mUnknown18(0), mUnknown38(0) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown24 = dynamic_cast<Rnd::Blur *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pts_exit%d.blur", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown1c = dynamic_cast<Rnd::View *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pts_exit%d.view", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown20 = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pts_exit%d.txt", pszLayout, nIndex))));

    mUnknown3c.SetAnim(mUnknown1c);
    mUnknown3c.Play(kExitRestFrame, kExitRestFrame);

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown28 = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s pts%d.txt", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown2c = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s ptsmult%d.txt", pszLayout, nIndex))));

    mUnknown30 = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD ptstmp.mat")));
    mUnknown34 = dynamic_cast<Rnd::Mat *>(Rnd::g_manager.Find(HxStr("HUD ptstmphot.mat")));

    mUnknown28->SetShowing(0);
    mUnknown2c->SetShowing(0);
}

void HudPoints::SetPoints(int nPoints) {
    mUnknown10 = nPoints;
    mUnknown28->SetText(HxStr(FormatString("%d", nPoints)));
    mUnknown28->SetShowing(1);
    mUnknown00 = 0.0f;
    mUnknown14 = 1;
    mUnknown08 = 0.0f;
}

void HudPoints::SetMultiplier(int nMultiplier) {
    mUnknown0c = nMultiplier;
    mUnknown2c->SetText(HxStr(FormatString("x%d", nMultiplier)));
    mUnknown2c->SetShowing(mUnknown0c > 1);
}
