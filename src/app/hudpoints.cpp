#include "app/hudpoints.h"

#include <cstring>

#include "app/overlay.h"
#include "math/color.h"
#include "math/vector3.h"
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

// The pulse and the flash each map 0 through 1 onto half through full.
constexpr float kHalf = 0.5f;

// How far the flash and the pulse fall each frame.
constexpr float kFlashDecay = 0.2f;
constexpr float kPulseDecay = 0.05f;

// Frames of the exit animation that ShowExit() and Bank() play.
constexpr float kShowExitFrom = 0.0f;
constexpr float kShowExitTo = 100.0f;
constexpr float kBankFrom = 200.0f;
constexpr float kBankTo = 300.0f;

} // namespace

// 0x00418818
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

// 0x00418de8
void HudPoints::SetFrame(float flTime) {
    mExitView->SetShowing(mExit.Update(flTime));

    // The points text is scaled across and in depth by the pulse. Only the three basis rows are
    // replaced, and the translation row is left as it was.
    const float flScale = mPulse * kHalf + kHalf;
    const Vector3 basis[] = {
        {flScale, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, flScale, 1.0f},
    };
    std::memcpy(mPointsText->mLocalXfm, basis, sizeof(basis));
    mPointsText->mDirty = 1;

    const float flBrightness = mFlash * kHalf + kHalf;
    mPointsText->SetColor(Color{flBrightness, flBrightness, flBrightness, 1.0f});

    mMultiplierText->SetColor(mHot != 0 ? mHotMat->mEmissive : mPlainMat->mEmissive);

    mFlash -= kFlashDecay;
    if (mFlash < 0.0f) {
        mFlash = 0.0f;
    }
    mPulse -= kPulseDecay;
    if (mPulse < mPulseRest) {
        mPulse = mPulseRest;
    }
}

// 0x00418f90
void HudPoints::ShowExit(int nPoints) {
    mPointsText->SetShowing(0);
    mShowing = 0;
    mFlash = 1.0f;
    mPulseRest = 0.0f;
    mExitText->SetText(HxStr(FormatString("%d", nPoints)));
    mExit.Play(kShowExitFrom, kShowExitTo);
    if (mExitBlur != nullptr) {
        mExitBlur->mXfms.clear();
    }
}

// 0x004190a8
void HudPoints::Bank() {
    if (mShowing != 0) {
        mExitText->SetText(HxStr(FormatString("%d", mPoints)));
        mExit.Play(kBankFrom, kBankTo);
        if (mExitBlur != nullptr) {
            mExitBlur->mXfms.clear();
        }
    }
    mPointsText->SetShowing(0);
    mShowing = 0;
    mPulseRest = 0.0f;
}

// 0x00429fe0
void HudPoints::SetPoints(int nPoints) {
    mPoints = nPoints;
    mPointsText->SetText(HxStr(FormatString("%d", nPoints)));
    mPointsText->SetShowing(1);
    mFlash = 0.0f;
    mShowing = 1;
    mPulseRest = 0.0f;
}

// 0x0042a0c8
void HudPoints::SetMultiplier(int nMultiplier) {
    mMultiplier = nMultiplier;
    mMultiplierText->SetText(HxStr(FormatString("x%d", nMultiplier)));
    mMultiplierText->SetShowing(mMultiplier > 1);
}
