#pragma once

#include "os/hxstr.h"

/**
 * One titled interval the profiler accumulates.
 *
 * The class is not polymorphic and has no RTTI, so its title is inferred. Recovery has barely
 * started. The title occupies `+0x00` and the record is 0x14 bytes, both measured from the four
 * assignments in MainLoop's constructor.
 */
struct ProfileTimer {
    /** Title the profiler reports this interval under. `+0x00` */
    HxStr mName;

    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
};

/**
 * Accumulator for the game's named intervals.
 *
 * The class is not polymorphic and has no RTTI, so its title is inferred from what its one caller
 * does. Recovery has barely started. The timer array starts at `+0x08`, measured from MainLoop's
 * constructor, which titles the first four records; the two words ahead of it are unrecovered and
 * the array length is unknown. The subsystem that owns the object lives around `0x0049c860`.
 *
 * This declaration exists to satisfy the references from MainLoop, which titles four timer records
 * in its constructor and samples the clock three times in its frame path.
 */
class Profiler {
public:
    int mUnknown00;          // +0x00
    int mUnknown04;          // +0x04
    ProfileTimer mTimers[4]; // +0x08
};

/**
 * The one profiler the game accumulates into.
 *
 * @ghidraAddress 0x00720378
 */
extern Profiler *g_pProfiler;

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
