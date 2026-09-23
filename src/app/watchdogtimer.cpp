#include "app/watchdogtimer.h"

#include "app/watchdog.h"

long long WatchdogTimer::Now() {
    if (mHasOrigin == 0) {
        return mUnknown08;
    }
    return mWatchdog->mNowNs + mNegatedOrigin;
}
