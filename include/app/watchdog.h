#pragma once

#include "app/watchdogclock.h"

/**
 * Scheduler that runs queued commands when their due time arrives.
 *
 * The class is not polymorphic and has no RTTI, and no literal in the image titles it. The title
 * here is retained from an earlier pass rather than attested. Only four `Sch` names exist anywhere
 * in the program, and none of them is a scheduler, so a replacement would be invention.
 *
 * What the class does is measured rather than inferred. The member at `+0x00` is the one pointer of
 * a red-black tree of Sch::TimedCommand pointers, whose header node the constructor takes from the
 * container pool at `0x00667080` and self-links through `+0x08` and `+0x0c`. The ordering predicate
 * at `0x004ac4f8` reads the due tick as a signed 64-bit quantity and falls back to the order member
 * as unsigned.
 *
 * Service() walks that tree from the left, stops as soon as the front entry is still in the future,
 * runs the wrapper it takes, clears the command's queued flag, and releases the wrapper. It does
 * not observe progress and it reports nothing.
 *
 * The threshold of six seconds in Service() is not a stall limit. When the distance from the last
 * reading exceeds it, the clock is marked forward and re-read, which stops the loop from running
 * six seconds of arrears in one burst.
 *
 * The member at `+0x0c` is a stream mode, one for recording and two for playback, and `+0x10` is a
 * one-word box for the stream that `0x004ac8c0` installs. The member at `+0x48` blocks every
 * queueing path while it is set.
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
     * Run every queued command whose due time has arrived.
     *
     * The loop takes the leftmost entry of the queue, stops once that entry is still in the future,
     * erases it, copies its due tick into the scheduler's current time, dispatches
     * Sch::TimedCommand::Run(), clears the command's queued flag, and releases the wrapper.
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
     * Copy the command queue so that it can be walked safely.
     *
     * @ghidraAddress 0x004a9a78
     */
    void Snapshot();

    /** Clock every due time is measured against. `+0x20` */
    WatchdogClock mClock;

private:
    // WatchdogTimer::Now() reads mNowNs.
    friend class WatchdogTimer;

    int mUnknown00;   // +0x00 the red-black tree of Sch::TimedCommand pointers, one pointer
    int mUnknown04;   // +0x04
    int mUnknown08;   // +0x08
    int mStreamMode;  // +0x0c 1 while recording, 2 while playing back; released by Close()
    int mUnknown10;   // +0x10 the one-word box for the installed stream; released by Close()
    int mUnknown14;   // +0x14 released by Close()
    long long mNowNs; // +0x18 the due tick of the command most recently run
    int mBlocked;     // +0x48 blocks every queueing path while set
    int mUnknown4c;   // +0x4c
};
