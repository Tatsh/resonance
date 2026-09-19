#pragma once

/** EE clock in cycles per millisecond, which is the divisor the elapsed time is computed with. */
constexpr unsigned kCyclesPerMillisecond = 294912;

/**
 * Read the free-running cycle counter.
 *
 * Every caller in the image inlines the read, so the image has no out-of-line copy and there is no
 * address to record. On the shipped target the read is the EE `Count` coprocessor register, which
 * is the one genuinely machine-specific part of the timing instrumentation, so a port supplies its
 * own implementation file for this declaration.
 *
 * @return The counter, which wraps.
 */
unsigned ReadCycleCount();

/**
 * Cycles accumulated across every read.
 *
 * Accumulating differences rather than counter readings is what makes the total survive a wrap of
 * the 32-bit counter.
 *
 * @ghidraAddress 0x007082c0
 */
extern long long g_llTotalCycles;

/**
 * The counter reading the last call took.
 *
 * @ghidraAddress 0x007082c8
 */
extern unsigned g_nLastCycleCount;

/**
 * The difference the last call added to the total.
 *
 * Every caller writes this word and none reads it, so it exists for a debugger rather than for the
 * game.
 *
 * @ghidraAddress 0x007082cc
 */
extern unsigned g_nLastCycleDelta;

/**
 * Report the milliseconds elapsed since the machine started.
 *
 * The body is here rather than in a source file because around thirty routines across the engine
 * inline it, among them MainLoop::PumpTimers(), WatchdogClock::Mark(), and the per-frame slots of
 * MetRenderer. Only one out-of-line copy exists, at the address below, and the two routines in the
 * asynchronous file layer are its only callers.
 *
 * The result is returned as an `int` and widened by its callers, which puts the ceiling at about 24
 * days of running time.
 *
 * The name is inferred from the arithmetic. Nothing in the image attests it.
 *
 * @return The elapsed milliseconds.
 * @ghidraAddress 0x00466300
 */
inline int GetElapsedMilliseconds() {
    const unsigned nCount = ReadCycleCount();
    g_nLastCycleDelta = nCount - g_nLastCycleCount;
    g_nLastCycleCount = nCount;
    g_llTotalCycles += g_nLastCycleDelta;
    return static_cast<int>(g_llTotalCycles / kCyclesPerMillisecond);
}
