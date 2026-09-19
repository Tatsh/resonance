#pragma once

/**
 * Clock the scheduler reads its due times from, in nanoseconds.
 *
 * The class is not polymorphic and has no RTTI, and its title is inferred from its one owner. Only
 * the member this reconstruction reads is recovered; the rest of the 0x28-byte layout is recorded
 * as unknown words so that the offsets survive.
 *
 * The clock's own reading is in milliseconds and Now() returns nanoseconds. The scale is not a
 * mystery constant: it is the double at `0x008263f8` divided by the 1000 that `0x00466360` returns,
 * which is one million nanoseconds to the millisecond.
 *
 * mOriginMs is public because MainLoop::Poll() reads it directly and the image has no accessor for
 * it. It sits amid unrecovered words, so the members below are grouped by access rather than by
 * offset, and each trailing comment records the real offset.
 */
class WatchdogClock {
public:
    /**
     * @ghidraAddress 0x00512498
     */
    WatchdogClock();

    /**
     * Read the clock.
     *
     * @return The current reading in nanoseconds.
     * @ghidraAddress 0x005125e0
     */
    long long Now();

    /**
     * Record a reference reading for the next comparison.
     *
     * @param nNanoseconds The reading to record.
     * @ghidraAddress 0x00512538
     */
    void Mark(long long nNanoseconds);

    /** The millisecond reading the current run started at. `+0x10` */
    long long mOriginMs;

private:
    double mUnknown00;    // +0x00
    int mUnknown08;       // +0x08
    int mUnknown0c;       // +0x0c
    int mUnknown18;       // +0x18 set to 1 on construction
    int mUnknown1c;       // +0x1c
    long long mUnknown20; // +0x20 cleared on construction
};
