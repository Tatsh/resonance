#include "app/watchdogplayback.h"

#include "app/watchdog.h"

// 0x00594968
WatchdogPlayback::WatchdogPlayback(Watchdog *pWatchdog) : mWatchdog(pWatchdog) {
}

// 0x005962e8
void WatchdogPlayback::Start() {
    mWatchdog->mClock.Pause();
    mWatchdog->RestartClock();
    QueueRemaining();
    mWatchdog->mClock.Resume();
}

// 0x00596330
void WatchdogPlayback::QueueRemaining() {
    while (mCursor != mCommands.end()) {
        mWatchdog->QueueReplayed(*mCursor);
        ++mCursor;
    }
}
