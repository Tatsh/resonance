#pragma once

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
 */
struct Tick {
    long long mValue; /*!< The count. +0x00 */
};

} // namespace Sch
