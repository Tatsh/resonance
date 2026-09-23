#include "met/metrenderer.h"

#include <algorithm>

#include "app/application.h"
#include "app/watchdog.h"
#include "met/metcommandrepeater.h"
#include "met/metscreen.h"
#include "profile/profiler.h"
#include "rnd/animatable.h"
#include "rnd/drawable.h"
#include "rnd/transformable.h"
#include "script/configquery.h"

namespace {

constexpr long long kNanosecondsPerMillisecond = 1000000;

// Half a millisecond, added before the division that turns a nanosecond interval into
// milliseconds so that the quotient rounds rather than truncates.
constexpr long long kHalfMillisecondNs = 500000;

constexpr float kMillisecondsPerSecond = 1000.0f;

// Frame position the front end rewinds to when it starts running.
constexpr float kFirstFrame = 1.0f;

// Configuration code the start-up value is read under.
constexpr int kStartUpConfigCode = 0x193;

// Reading of the frame clock in nanoseconds, measured from the origin the watchdog's clock
// recorded when the run started. MainLoop has its own copy of the same inline.
inline long long FrameClockNs(Watchdog *pWatchdog) {
    return (ProfileClockMilliseconds() - pWatchdog->mClock.mOriginMs) * kNanosecondsPerMillisecond;
}

// Milliseconds between two frame-clock readings, rounded rather than truncated.
inline int FrameIntervalMs(long long nNowNs, long long nThenNs) {
    return static_cast<int>((nNowNs - nThenNs + kHalfMillisecondNs) / kNanosecondsPerMillisecond);
}

} // namespace

void MetRenderer::OnUnknownSlot4() {
    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }

    mUnknowna8 = 1;
    mUnknown68 = kFirstFrame;
    mUnknown70 = FrameClockNs(Application::shared()->GetWatchdog());
    QueryConfigValue(kStartUpConfigCode); // Yes, the binary discards the result.
}

void MetRenderer::OnUnknownSlot9() {
    if (mUnknowna8 == 0) {
        return;
    }

    const long long nNowNs = FrameClockNs(Application::shared()->GetWatchdog());
    const int nIntervalMs = FrameIntervalMs(nNowNs, mUnknown70);

    mUnknown70 = nNowNs;
    mUnknown68 += mUnknown64 * static_cast<float>(nIntervalMs) / kMillisecondsPerSecond;

    for (std::vector<MetScreen *>::iterator it = mUnknown84.begin(); it != mUnknown84.end(); ++it) {
        (*it)->UpdateAnimationFrame(mUnknown68);

        if (mUnknowna8 == 0) {
            return;
        }

        // A screen that pushed or popped another one invalidated the iterator, so the walk is
        // abandoned rather than restarted. The screens past the change are not advanced this
        // frame.
        if (mUnknown98 != 0) {
            mUnknown98 = 0;
            break;
        }
    }

    mUnknown9c->SetFrame(mUnknown68);
    mUnknown9c->UpdateWorldXfm(nullptr, 0); // Yes, the binary discards the result.
}

void MetRenderer::OnFadeOutDone() {
    mUnknownd0 = 0;

    if (mUnknownc8 != 0) {
        return;
    }

    MetScreen *const pPending = mUnknownc4;

    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }

    mUnknown7c = pPending;
    mUnknown80 = 1;
    AddScreen(mUnknown7c);
    mUnknown7c->PollContainerLoad(); // Yes, the binary discards the result.
    mUnknown7c->mUnknown4c = 1;
    mUnknown7c->mUnknown50 = 1;
}

void MetRenderer::OnFadeInDone() {
}

void MetRenderer::SetActivePanel(MetScreen *pScreen) {
    mUnknown7c = pScreen;

    if (mUnknown94 != nullptr) {
        mUnknown94->Reset();
    }
}

void MetRenderer::AddScreen(MetScreen *pScreen) {
    if (std::find(mUnknown84.begin(), mUnknown84.end(), pScreen) != mUnknown84.end()) {
        return;
    }

    mUnknown84.push_back(pScreen);
    mUnknown98 = 1;
}

void MetRenderer::RemoveScreenView(Rnd::View *pView) {
    mUnknowna0->RemoveTrans(pView);
    mUnknowna0->RemoveDraw(pView);
    mUnknowna0->RemoveAnim(pView);
}

void MetRenderer::ClearScreenScene() {
    mUnknowna0->ReleaseAnimsRefs();
    mUnknowna0->ClearDraws();
    mUnknowna0->ClearTransList();
}

void MetRenderer::ClearBackgroundScene() {
    mUnknowna4->ReleaseAnimsRefs();
    mUnknowna4->ClearDraws();
    mUnknowna4->ClearTransList();
}
