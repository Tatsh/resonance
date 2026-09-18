#pragma once

#include "app/watchdogclock.h"

/**
 * Monitor for the operations that run outside the frame loop.
 *
 * The class is not polymorphic and has no RTTI, so its title is inferred from what its members do.
 * The object is 0x50 bytes, stores a list of outstanding operations, and Service() reports an
 * operation that has made no progress for roughly six seconds. MainLoop drives Service() from one
 * of its two periodic timers and installs the poll and redraw callbacks that a long operation
 * calls back into.
 *
 * Only the member this reconstruction reads is recovered. The list at `+0x00` is one pointer under
 * the SGI layout, addressing a self-linked node the constructor takes from the STL pool, and that
 * node is 0x18 bytes rather than the usual 0x10, which makes its value 16 bytes wide. The three
 * pointers Close() releases and the trailing words are recorded as unknown so that their offsets
 * survive.
 *
 * mClock is public because MainLoop::Poll() reads its origin directly and the image has no
 * accessor for it. It sits amid unrecovered words, so the members below are grouped by access
 * rather than by offset, and each trailing comment records the real offset.
 */
class Watchdog {
public:
    /**
     * @ghidraAddress 0x004a9858
     */
    Watchdog();

    /**
     * Release the monitor's buffers.
     *
     * @ghidraAddress 0x004ac9e8
     */
    void Close();

    /**
     * Advance the monitor and report a stalled operation.
     *
     * @ghidraAddress 0x004aa848
     */
    void Service();

    /**
     * Write the accumulated readings out and restart the measurement.
     *
     * @ghidraAddress 0x004aca60
     */
    void Flush();

    /**
     * Copy the list of outstanding operations so that it can be walked safely.
     *
     * @ghidraAddress 0x004a9a78
     */
    void Snapshot();

    /** Clock the monitor measures every operation against. `+0x20` */
    WatchdogClock mClock;

private:
    int mUnknown00;    // +0x00 the outstanding-operation list, one pointer
    int mUnknown04;    // +0x04
    int mUnknown08;    // +0x08
    int mUnknown0c;    // +0x0c released by Close()
    int mUnknown10;    // +0x10 released by Close()
    int mUnknown14;    // +0x14 released by Close()
    long long mLastNs; // +0x18 reading the current operation last made progress at
    int mUnknown48;    // +0x48
    int mUnknown4c;    // +0x4c
};
