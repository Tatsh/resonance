#pragma once

#include "os/timer.h"

/** EE clock in cycles per millisecond, which is the divisor the elapsed time is computed with. */
constexpr unsigned kCyclesPerMillisecond = 294912;

/** Milliseconds in one second, the rate GetMillisecondsPerSecond() reports. */
constexpr int kMillisecondsPerSecond = 1000;

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
 * Report the milliseconds elapsed since the machine started.
 *
 * The body is here rather than in a source file because around thirty routines across the engine
 * inline it, among them MainLoop::PumpTimers(), Sch::SystemTime::Mark(), and the per-frame slots of
 * MetRenderer. Only one out-of-line copy exists, at the address below, and the two routines in the
 * asynchronous file layer are its only callers.
 *
 * The result is returned as an `int` and widened by its callers, which puts the ceiling at about 24
 * days of running time.
 *
 * The name is inferred from the arithmetic. Nothing in the image attests it.
 *
 * @return The elapsed milliseconds.
 * @ghidraAddress NTSC-U/C: 0x00466300
 * @ghidraAddress PAL: 0x004a3d30
 */
inline int GetElapsedMilliseconds() {
    const unsigned nCount = ReadCycleCount();
    Timer::sLastCycleDelta = nCount - Timer::sElapsedTimer;
    Timer::sElapsedTimer = nCount;
    Timer::sElapsedCycles += Timer::sLastCycleDelta;
    return static_cast<int>(Timer::sElapsedCycles / kCyclesPerMillisecond);
}

/**
 * Report how many units GetElapsedMilliseconds() counts in one second.
 *
 * The body is here rather than in a source file for the reason recorded on
 * GetElapsedMilliseconds(). Its one out-of-line copy follows that routine's copy directly, in the
 * unit whose static initialiser is at `0x004662d0` and which also defines ShowReportedMessage().
 * HudScreenFlash's constructor calls that copy, and Sch::SystemTime divides its scale by the
 * result.
 *
 * The name is inferred from the value. Nothing in the image attests it.
 *
 * @return 1000.
 * @ghidraAddress NTSC-U/C: 0x00466360
 * @ghidraAddress PAL: 0x004a3d90
 */
inline int GetMillisecondsPerSecond() {
    return kMillisecondsPerSecond;
}
