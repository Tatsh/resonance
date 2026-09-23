#include "app/watchdogclock.h"

#include "os/cycles.h"

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
