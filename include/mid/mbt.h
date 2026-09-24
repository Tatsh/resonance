#pragma once

#include <iostream>

class IBStream;
class OBStream;

/**
 * Position the printer renders as positive infinity.
 *
 * GemPacket initialises its own position to this value. Mid::MBT::Print() renders any value above
 * `0x2aaaaaa8` as `[inf]tk`, so the name is inferred from the one stored value inside that range
 * rather than from a comparison against the constant.
 */
constexpr int kMBTInfinity = 0x2aaaaaab;

/** Largest finite position, from the bound IsFiniteMBT() accepts. */
constexpr int kMBTMaximum = 0x2aaaaaaa;

/** Smallest finite position, from the bound IsFiniteMBT() accepts. */
constexpr int kMBTMinimum = -0x2aaaaaaa;

/**
 * Report whether a position is finite.
 *
 * The body adds 0x2aaaaaaa to the position and compares the sum against 0x55555554 as an unsigned
 * value, which accepts the closed range from kMBTMinimum to kMBTMaximum in one test. Several
 * callers discard the result, which is the shape of an assertion compiled without its report.
 *
 * The bound here and the bound Mid::MBT::Print() applies differ by two. Print() renders any value
 * above `0x2aaaaaa8` as infinite while this test accepts up to `0x2aaaaaaa`, so two positions are
 * finite by this test and infinite to the printer.
 *
 * The one emission sits in the unit at `0x00100040`, between the Sequencer template and
 * SequencerCmd, and every caller reaches that copy.
 *
 * @param nTick The position, in MIDI ticks.
 * @return Non-zero for a finite position.
 * @ghidraAddress 0x00100ab8
 */
int IsFiniteMBT(int nTick);

namespace Mid {

/**
 * Song position in MIDI ticks, printed as measure, beat, and tick.
 *
 * The object is four bytes and the class is not polymorphic, so it emits no vtable and no RTTI. The
 * image never records this type's name, and the name Mid::MBT is retained by convention only.
 *
 * The image's `Q23Mid3MBT` descriptor belongs to a different, unused class. That class is 0x10
 * bytes, with a one-based measure at `+0x0`, a one-based beat at `+0x4`, the tick within the beat
 * at `+0x8`, and a vptr at `+0xc`. Its vtable at `0x008110e8` has the accessor at `0x003d63f8` in
 * slot 0 and a Print() at `0x003d67f0` in slot 1 that writes `[measure:beat:tick]`. Its inline
 * constructor at `0x003d6798` splits a tick count by beats per measure and ticks per beat. No code
 * refers to the constructor, the Print(), or the vtable. The tree cannot declare both classes under
 * the one name Mid::MBT, so only this four-byte word is reconstructed.
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
 * Sch::CmdID and Sch::Tick are both distinct from this class and are easy to confuse with it,
 * because all three transfer one four-byte lvalue through one Write() or one Read(). The three
 * Print() bodies separate them. CmdID::Print writes ` {cmdID `, Sch::Tick::Print divides by
 * 1000000000.0 and appends `s`, and this class writes the three-part form above. The transfer
 * alone cannot separate the three types. CatchProgressPacket and TrackSelectPacket each stream a
 * member through the emission at `0x004acf28` and print the same member through Print() here,
 * which settles both members as this class.
 *
 * The three addresses below are the emission the scheduler translation unit uses, and a second
 * emission of Save() exists elsewhere, which is the shape of an inline member rather than of an
 * ordinary out-of-line one. The bodies are defined out of line here, keeping the two stream
 * classes forward-declared for the many headers that include this one.
 *
 * The one member is public, because the readers of a position apply arithmetic to it directly and
 * the image exposes no accessor.
 */
class MBT {
public:
    /**
     * Start the position at kMBTInfinity.
     *
     * Inline, and expanded wherever an object holding a position is constructed. Every New()
     * factory whose class has a member of this type stores `0x2aaaaaab` into that member and into
     * no other field. NoteMsg::New() at `0x003d6d80`, EraseMsg::New() at `0x003d6b28`, and
     * SeekerMsg::New() at `0x003d6f10` are three of them, at three different member offsets.
     */
    MBT() : mTick(kMBTInfinity) {
    }

    /**
     * Wrap a tick count, checking that it is finite.
     *
     * Inline. The check is an IsFiniteMBT() call whose result is discarded, the shape of an
     * assertion compiled without its report, and it sits beside the store of the same value at
     * every site that builds a position from a plain count. NoteMsg::Load() at `0x003e3890` is one,
     * and the gameplay classes Phrase, GsPeriodical, TrackData, Catcher, and PhraseMaker show the
     * same pairing.
     *
     * @param nTick The position, in MIDI ticks.
     */
    explicit MBT(int nTick) : mTick(nTick) {
        IsFiniteMBT(nTick);
    }

    /**
     * Write the position.
     *
     * @param stream The stream to write to.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress 0x004acf28
     */
    OBStream &Save(OBStream &stream);

    /**
     * Read the position back.
     *
     * @param stream The stream to read from.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress 0x004acf68
     */
    IBStream &Load(IBStream &stream);

    /**
     * Write the position to a diagnostic stream as measure, beat, and tick.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x004ace18
     */
    void Print(std::ostream &stream);

    /**
     * The position, in MIDI ticks at 480 per quarter note.
     *
     * +0x00
     */
    int mTick;
};

} // namespace Mid
