#include "app/watchdogclock.h"

#include "os/cycles.h"

namespace {

// The factor Advance() scales its argument by before dividing by the double at +0x00.
constexpr double kAdvanceScale = 1000000.0;

// Nanoseconds in one second. Divided by GetMillisecondsPerSecond(), it gives the nanoseconds in one
// clock unit.
constexpr double kNanosecondsPerSecond = 1000000000.0;

} // namespace

// 0x00512498
WatchdogClock::WatchdogClock() {
    mPausedRunMs = 0;
    mRunning = 1;
    mUnknown00 = kNanosecondsPerSecond / static_cast<double>(GetMillisecondsPerSecond());
    const long long nNowMs = GetElapsedMilliseconds();
    mOriginMs = nNowMs;
    mStartMs = nNowMs;
}

// 0x00512538
void WatchdogClock::Mark(long long nNanoseconds) {
    const long long nMarkMs = static_cast<long long>(nNanoseconds / mUnknown00);
    if (mRunning != 0) {
        mStartMs = GetElapsedMilliseconds() - nMarkMs;
    } else {
        mPausedRunMs = nMarkMs;
    }
}

// 0x005125e0
long long WatchdogClock::Now() {
    long long nRunMs;
    if (mRunning != 0) {
        nRunMs = GetElapsedMilliseconds() - mStartMs;
    } else {
        nRunMs = mPausedRunMs;
    }
    return static_cast<long long>(nRunMs * mUnknown00);
}

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
