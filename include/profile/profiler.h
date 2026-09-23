#pragma once

#include <vector>

#include "os/hxstr.h"

/**
 * One titled interval the profiler accumulates, 0x14 bytes.
 *
 * The class is not polymorphic and has no RTTI, so its title is inferred. The static initialiser
 * at `0x0053d700` builds each record by copying a temporary whose first word it never writes, so
 * mStartCycles begins undefined. GfxDevice::DrawSubsystemTimingGraph() reports each record's name
 * and mCycles, and the frame timer reset at `0x0053dcf8` writes all three counters of its own
 * record.
 */
struct ProfileTimer {
    /** EE `Count` when the interval was last entered. `+0x00` */
    unsigned int mStartCycles;
    /** Cycles accumulated across the frame. `+0x04` */
    unsigned int mCycles;
    /** Title the profiler reports this interval under. `+0x08` */
    HxStr mName;
    /** Nesting depth of the interval, which the reset returns to 1. `+0x10` */
    int mDepth;
};

/**
 * The timers the game accumulates into during a frame, twenty records.
 *
 * MainLoop titles the first four, and GfxDevice::DrawSubsystemTimingGraph() draws one bar per
 * record.
 *
 * @ghidraAddress 0x00720378
 */
extern std::vector<ProfileTimer> g_profileTimers;

/**
 * The previous frame's copy of the timers, twenty records.
 *
 * GfxDevice::BeginFrame() passes it by address, and the frame-rate readouts take the frame time
 * from record 19 and a second interval from record 18.
 *
 * @ghidraAddress 0x00720388
 */
extern std::vector<ProfileTimer> g_lastFrameProfileTimers;

/**
 * Milliseconds per EE cycle, `1.0f / 294912.0f`, which the frame timer reset stores.
 *
 * @ghidraAddress 0x00720394
 */
extern float g_flCyclesToMilliseconds;

/**
 * The frame interval ResetFrameTimer() restarts, a record outside both tables.
 *
 * The name is inferred.
 *
 * @ghidraAddress 0x007203a0
 */
extern ProfileTimer g_frameTimer;

/**
 * Doubleword ResetFrameTimer() clears. The image has no reader, and the name is inferred.
 *
 * @ghidraAddress 0x00720398
 */
extern long long g_llFrameTimerCycles;

/**
 * Store g_flCyclesToMilliseconds and restart g_frameTimer from the current EE `Count`.
 *
 * The timer is first left as a nested interval is. Its depth drops by one, and when that reaches
 * zero the cycles since mStartCycles are added to mCycles. The reset then clears mCycles, which
 * discards that addition, sets the depth to 1, records the current `Count`, and clears
 * g_llFrameTimerCycles. Rnd::Manager::Init() is the one caller. The name is the analysis
 * program's.
 *
 * @ghidraAddress 0x0053dcf8
 */
void ResetFrameTimer();

/**
 * Sample the wall clock the profiler counts against.
 *
 * The routine accumulates the EE cycle counter into a 64-bit total and divides by the cycles per
 * millisecond. It is inline in the profiler's own header, so the compiler expanded a copy into
 * each of MainLoop::Poll(), MainLoop::PumpTimers(), and MainLoop::KeepAliveDraw() rather than
 * emitting one body.
 *
 * @return Milliseconds since the counter started.
 */
long long ProfileClockMilliseconds();
