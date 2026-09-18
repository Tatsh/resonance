#pragma once

#include "app/watchdog.h"

/**
 * Time base that a Watchdog's readings are taken against.
 *
 * The class is not polymorphic and has no RTTI, and its title is inferred from its one owner and
 * its one argument. The origin is stored negated and may be set only once, so a second SetOrigin()
 * is ignored. Every member is private, because the only code that reads one is a member of this
 * class.
 */
class WatchdogTimer {
public:
    /**
     * @param pWatchdog The monitor these readings belong to.
     * @ghidraAddress 0x004a7780
     */
    WatchdogTimer(Watchdog *pWatchdog);

    /**
     * Fix the origin of the time base.
     *
     * The first call wins. Every later call is ignored.
     *
     * @param nNanoseconds The reading to treat as zero.
     * @ghidraAddress 0x004a7828
     */
    void SetOrigin(long long nNanoseconds);

private:
    long long mNegatedOrigin; // +0x00
    long long mUnknown08;     // +0x08
    int mHasOrigin;           // +0x10
    Watchdog *mWatchdog;      // +0x14
};
