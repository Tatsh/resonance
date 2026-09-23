#include "app/watchdogtimer.h"

#include "app/watchdog.h"

long long WatchdogTimer::Now() {
    if (mHasOrigin == 0) {
        return mUnknown08;
    }
    return mWatchdog->mNowNs + mNegatedOrigin;
}

// 0x004a7878
void WatchdogTimer::Pause() {
    if (mHasOrigin != 0) {
        mHasOrigin = 0;
        mUnknown08 = mNegatedOrigin + mWatchdog->mNowNs;
    }
}
