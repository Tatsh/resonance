#include "app/watchdogclock.h"

#include "os/cycles.h"

namespace {

// The factor Advance() scales its argument by before dividing by the double at +0x00.
constexpr double kAdvanceScale = 1000000.0;

} // namespace

// 0x00512680
void WatchdogClock::Pause() {
    if (mRunning == 0) {
        return;
    }

    const long long nNowMs = GetElapsedMilliseconds();
    mRunning = 0;
    mPausedRunMs = nNowMs - mStartMs;
}

// 0x00512700
void WatchdogClock::Resume() {
    if (mRunning != 0) {
        return;
    }

    const long long nNowMs = GetElapsedMilliseconds();
    mRunning = 1;
    mStartMs = nNowMs - mPausedRunMs;
}

// 0x00512788
void WatchdogClock::Advance(int nAmount) {
    Pause();
    mPausedRunMs += static_cast<long long>(nAmount * kAdvanceScale / mUnknown00);
}
