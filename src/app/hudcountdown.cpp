#include "app/hudcountdown.h"

#include "app/application.h"
#include "app/overlay.h"
#include "game/gamemanagerimpl.h"
#include "os/formatstring.h"
#include "os/hxstr.h"
#include "rnd/blur.h"
#include "rnd/manager.h"
#include "rnd/text.h"
#include "rnd/transanim.h"
#include "script/configquery.h"

namespace {

// Count the constructor starts with, which no real count matches.
constexpr int kNoCount = -123;

// Frame the constructor starts with.
constexpr float kNoFrame = 1.0e9f;

// Configuration code that disables the countdown.
constexpr int kDisplayModeConfigCode = 0x3a1;

// MIDI ticks in one bar, and how far ahead of the frame a bar boundary counts.
constexpr float kTicksPerBar = 1920.0f;
constexpr float kBarLead = 100.0f;

// Largest count the countdown shows.
constexpr unsigned kLargestCount = 9;

} // namespace

HudCountdown::HudCountdown(int nIndex, int nTargetBar)
    : mUnknown00(nTargetBar), mUnknown04(kNoCount), mUnknown08(kNoFrame) {
    const char *pszLayout =
        g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown0c = dynamic_cast<Rnd::TransAnim *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s countdown%d.tnm", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown10 = dynamic_cast<Rnd::Text *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s countdown%d.txt", pszLayout, nIndex))));

    pszLayout = g_hudLayoutName.mStr != nullptr ? g_hudLayoutName.mStr : g_szEmptyString;
    mUnknown14 = dynamic_cast<Rnd::Blur *>(
        Rnd::g_manager.Find(HxStr(FormatString("%s countdown%d.blur", pszLayout, nIndex))));

    mUnknown14->SetShowing(0);

    if (Application::shared()->GetPlayMode() == kPlayModeGame &&
        QueryConfigFlag(kDisplayModeConfigCode) == 0) {
        mUnknown18 = 0;
    } else {
        mUnknown18 = 1;
    }
}

void HudCountdown::SetFrame(float flFrame) {
    if (mUnknown18 != 0) {
        return;
    }

    const int nCount = mUnknown00 - static_cast<int>((flFrame + kBarLead) / kTicksPerBar);
    // One unsigned comparison tests 1 through 9, and a count of 0 wraps to the largest value.
    const bool bShowing = static_cast<unsigned>(nCount - 1) < kLargestCount;
    mUnknown14->SetShowing(bShowing);
    if (!bShowing) {
        return;
    }

    if (nCount != mUnknown04) {
        mUnknown04 = nCount;
        mUnknown10->SetText(HxStr(FormatString("%d", nCount)));
        mUnknown08 = flFrame;
    }
    mUnknown0c->SetFrame(flFrame - mUnknown08);
}
