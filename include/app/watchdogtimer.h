#pragma once

#include "app/watchdog.h"

/**
 * Time base a scheduler measures its due times against, and the base class of Sch::TickClock.
 *
 * The class is not polymorphic and has no RTTI, and no literal titles it. The title here is
 * retained from an earlier pass rather than attested, and it understates the class: this 0x18-byte
 * object is the base of Sch::TickClock, and the bodies at `0x004a7828`, `0x004a7848`, `0x004a7878`,
 * and `0x004a77c0` are single bodies shared between the two rather than routines of a monitor.
 * A replacement title would be invention, because the only `Sch` names in the image are the command
 * classes, the tick, and the clock itself.
 *
 * The origin is stored negated and may be set only once, so a second SetOrigin() is ignored. Every
 * member is private, because the only code that reads one is a member of this class.
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
