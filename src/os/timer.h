#pragma once

/**
 * The EE cycle clock the engine times itself against.
 *
 * Every member is static, and the class emits no RTTI. Its name comes from the debugging symbols of
 * the North American demo release. GetElapsedMilliseconds() accumulates into the members, and
 * around thirty routines across the engine inline GetElapsedMilliseconds().
 */
class Timer {
public:
    /**
     * Start the clock from the current counter reading.
     *
     * The total and the last difference become zero and the last reading becomes the counter. The
     * counter is read twice and the first reading is discarded. The static initialiser at
     * `0x004662d0`, in the unit that defines the out-of-line GetElapsedMilliseconds(), is the one
     * caller.
     *
     * @ghidraAddress NTSC-U/C: 0x004fefb0
     * @ghidraAddress PAL: 0x0053dd60
     */
    static void Init();

    /**
     * Milliseconds per EE cycle, the reciprocal of kCyclesPerMillisecond.
     *
     * Init() writes it and no routine in the image reads it.
     *
     * @ghidraAddress NTSC-U/C: 0x007082b4
     * @ghidraAddress PAL: 0x0074bde4
     */
    static float sClock2Ms;

    /**
     * Word Init() clears beside the cycle state. No routine in the image reads it.
     *
     * @ghidraAddress NTSC-U/C: 0x007082b8
     * @ghidraAddress PAL: 0x0074bde8
     */
    static int sElapsedMs;

    /**
     * Cycles accumulated across every read.
     *
     * Accumulating differences rather than counter readings makes the total correct across a wrap
     * of the 32-bit counter.
     *
     * @ghidraAddress NTSC-U/C: 0x007082c0
     * @ghidraAddress PAL: 0x0074bdf0
     */
    static long long sElapsedCycles;

    /**
     * The counter reading the last call took.
     *
     * @ghidraAddress NTSC-U/C: 0x007082c8
     * @ghidraAddress PAL: 0x0074bdf8
     */
    static unsigned sElapsedTimer;

    /**
     * The difference the last call added to the total.
     *
     * The word follows sElapsedTimer, and Init() addresses it through that member's base. Every
     * caller writes this word and none reads it.
     *
     * @ghidraAddress NTSC-U/C: 0x007082cc
     * @ghidraAddress PAL: 0x0074bdfc
     */
    static unsigned sLastCycleDelta;
};
