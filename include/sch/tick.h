#pragma once

#include <iostream.h>

#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Sch {

/**
 * Scheduler time, as a signed 64-bit count.
 *
 * The type has no RTTI descriptor, because it is not polymorphic. Its title comes from the mangled
 * constructor signature of `Catcher`. The RTTI records that signature as
 * `__7CatcherP9PhraseMgrP9QuantizerPC9TrackDataPQ23Sch9TickClockiGQ23Sch4Tick`, and `Sch::Tick` is
 * therefore the original title of a named type passed by value.
 *
 * The width is fixed by the code that moves one around. `Sch::TimedCommand` stores two of them,
 * and its constructor at `0x005d32f8` writes both with `sd`, while the scheduler run loop at
 * `0x004aa848` compares one against the clock with a 64-bit signed load and the queueing paths at
 * `0x004ac608` and `0x004ac698` compute one with `daddu` and `dsubu`. Eight bytes and 64-bit signed
 * arithmetic are therefore measured rather than assumed.
 *
 * Whether the original declared a `class` or a `struct` is undetermined, and so is the set of
 * arithmetic operators it declared. Every arithmetic use in the image is inlined into its caller.
 * No address therefore attaches to an operator, and none is declared here. The one member is
 * public because the scheduler applies 64-bit arithmetic to it directly and the image exposes no
 * accessor.
 *
 * The count is in nanoseconds, and three independent measurements agree. Print() below divides it
 * by the double 1000000000.0 at `0x0083c130` and appends the literal `s` at `0x0083c0e8`, so one
 * unit is one thousand-millionth of a second. Watchdog's clock scales its millisecond readings by
 * `1.0e9 / 1000.0` at `0x005124c8` to produce one. The run loop at `0x004aa8a4` treats a gap of
 * 6000000000 as six seconds of arrears.
 *
 * A song position is a separate quantity and is not one of these. Sch::TickClock::Tick() reports
 * a song position as a plain `int` in MIDI ticks at 480 per quarter note, and Sch::TempoMap
 * converts between the two.
 */
struct Tick {
    /**
     * Write the count.
     *
     * The body is not reconstructed, because two measurements of the member count disagree. Save()
     * reads the words at `+0x00` and `+0x04` with two separate four-byte loads at `0x006100c8` and
     * `0x006100e0` and writes each through its own OBStream::Write(), with no shift anywhere, which
     * is evidence of two 32-bit lvalues. Print() instead reads the whole value with one `ld` at
     * `0x00610064`, and the scheduler applies 64-bit `daddu`, `dsubu`, and signed `slt` to it,
     * which is evidence of one 64-bit lvalue. An anonymous union of a `long long` and a
     * two-element `int` array satisfies both, and that model is recorded here as a hypothesis
     * rather than written into the member below.
     *
     * @param stream The stream to write to.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress 0x006100a8
     */
    OBStream &Save(OBStream &stream);

    /**
     * Read the count back.
     *
     * The body is not reconstructed, for the reason recorded on Save(). Load() reads four bytes
     * into `+0x00` and four bytes into `+0x04` through two separate IBStream::Read() calls.
     *
     * @param stream The stream to read from.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress 0x00610118
     */
    IBStream &Load(IBStream &stream);

    /**
     * Write the count to a diagnostic stream as a number of seconds.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00610050
     */
    void Print(ostream &stream);

    long long mValue; /*!< The count, in nanoseconds. +0x00 */
};

} // namespace Sch
