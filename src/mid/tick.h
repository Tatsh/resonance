#pragma once

#include <iostream>

class IBStream;
class OBStream;

/**
 * Position the printer renders as positive infinity.
 *
 * GemPacket initialises its own position to this value. Sch::Tick::Print() renders any value above
 * `0x2aaaaaa8` as `[inf]tk`, so the name is inferred from the one stored value inside that range
 * rather than from a comparison against the constant.
 */
constexpr int kTickInfinity = 0x2aaaaaab;

/** Largest finite position, from the bound Sch::Tick::IsInRange() accepts. */
constexpr int kTickMaximum = 0x2aaaaaaa;

/** Smallest finite position, from the bound Sch::Tick::IsInRange() accepts. */
constexpr int kTickMinimum = -0x2aaaaaaa;

namespace Sch {

/**
 * Song position in MIDI ticks, printed as measure, beat, and tick.
 *
 * Its name comes from the debugging symbols of the North American demo release. The demo's
 * Print(), saveGuts(), restoreGuts(), and IsInRange() have the same instructions as this class's.
 *
 * The object is four bytes and the class is not polymorphic, so it emits no vtable and no RTTI. The
 * anonymous-namespace marker for the file-local commands of Catcher records the constructor
 * signature `Catcher(PhraseMgr *, Quantizer *, const TrackData *, Sch::TickClock *, int,
 * Sch::Tick)`, and that constructor stores the last parameter with one `sw`.
 *
 * The image's RTTI descriptor for `Mid::MBT` belongs to a different, unused class. That class is
 * 0x10 bytes, with a one-based measure at `+0x0`, a one-based beat at `+0x4`, the tick within the
 * beat at `+0x8`, and a vptr at `+0xc`. Its vtable at `0x008110e8` has the accessor at `0x003d63f8`
 * in slot 0 and a Print() at `0x003d67f0` in slot 1 that writes `[measure:beat:tick]`. Its inline
 * constructor at `0x003d6798` splits a tick count by beats per measure and ticks per beat. No code
 * refers to the constructor, the Print(), or the vtable.
 *
 * Print() fixes the units. It divides the word by 1920 for a measure, divides the remainder by 480
 * for a beat, and prints the second remainder as the tick, then appends `tk`. Measure and beat are
 * both incremented before printing and are therefore one-based, and 480 ticks per beat with four
 * beats per measure agrees with Sch::TickClock, which reports a song position as MIDI ticks at 480
 * per quarter note.
 *
 * Two sentinels bound the range. A word above `0x2aaaaaa8` prints `[inf]tk` and a word at or below
 * `-715827881` prints `[-inf]tk`, so a position outside those bounds is infinite rather than
 * numeric. GemPacket initialises its own member to `0x2aaaaaab`, which is inside the positive
 * sentinel range.
 *
 * Sch::CmdID and Sch::Time are both distinct from this class and are easy to confuse with it,
 * because all three stream through WriteLE() and ReadLE() with the same three-function shape. The
 * three Print() bodies separate them. CmdID::Print writes ` {cmdID `, Sch::Time::Print divides by
 * 1000000000.0 and appends `s`, and this class writes the three-part form above.
 * CatchProgressPacket and TrackSelectPacket each stream a member through the emission at
 * `0x004acf28` and print the same member through Print() here. Both members are therefore this
 * class.
 *
 * The three addresses below are the emission the scheduler translation unit uses, and a second
 * emission of saveGuts() exists elsewhere, which is the shape of an inline member rather than of an
 * ordinary out-of-line one. The bodies are defined out of line here, keeping the two stream
 * classes forward-declared for the many headers that include this one.
 *
 * The one member is public, because the readers of a position apply arithmetic to it directly and
 * the image exposes no accessor.
 */
class Tick {
public:
    /**
     * Report whether a position is finite.
     *
     * The body adds 0x2aaaaaaa to the position and compares the sum against 0x55555554 as an
     * unsigned value. The one test accepts the closed range from kTickMinimum to kTickMaximum.
     * Several callers discard the result, the shape of an assertion compiled without its report.
     *
     * The bound here and the bound Print() applies differ by two. Print() renders any value above
     * `0x2aaaaaa8` as infinite while this test accepts up to `0x2aaaaaaa`. Two positions are
     * therefore finite by this test and infinite to the printer.
     *
     * The one emission sits in the unit at `0x00100040`, between the Sequencer template and
     * SequencerCmd, and every caller calls that copy.
     *
     * @param nTick The position, in MIDI ticks.
     * @return Non-zero for a finite position.
     * @ghidraAddress NTSC-U/C: 0x00100ab8
     * @ghidraAddress PAL: 0x00100ab8
     */
    static int IsInRange(int nTick);

    /**
     * Start the position at kTickInfinity.
     *
     * Inline, and expanded wherever an object holding a position is constructed. Every New()
     * factory whose class has a member of this type stores `0x2aaaaaab` into that member and into
     * no other field. NoteMsg::New() at `0x003d6d80`, EraseMsg::New() at `0x003d6b28`, and
     * SeekerMsg::New() at `0x003d6f10` are three of them, at three different member offsets.
     */
    Tick() : mTick(kTickInfinity) {
    }

    /**
     * Wrap a tick count, checking that it is finite.
     *
     * Inline. The check is an IsInRange() call whose result is discarded, the shape of an
     * assertion compiled without its report, and it sits beside the store of the same value at
     * every site that builds a position from a plain count. NoteMsg::restoreGuts() at `0x003e3890`
     * is one, and the gameplay classes Phrase, GsPeriodical, TrackData, Catcher, and PhraseMaker
     * show the same pairing.
     *
     * @param nTick The position, in MIDI ticks.
     */
    explicit Tick(int nTick) : mTick(nTick) {
        IsInRange(nTick);
    }

    /**
     * Write the position.
     *
     * @param stream The stream to write to.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress NTSC-U/C: 0x004acf28
     * @ghidraAddress PAL: 0x004eb0c8
     */
    OBStream &saveGuts(OBStream &stream) const;

    /**
     * Read the position back.
     *
     * @param stream The stream to read from.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress NTSC-U/C: 0x004acf68
     * @ghidraAddress PAL: 0x004eb108
     */
    IBStream &restoreGuts(IBStream &stream);

    /**
     * Write the position to a diagnostic stream as measure, beat, and tick.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x004ace18
     * @ghidraAddress PAL: 0x004eafb8
     */
    void Print(std::ostream &stream) const;

    /**
     * The position, in MIDI ticks at 480 per quarter note.
     *
     * +0x00
     */
    int mTick;
};

} // namespace Sch
