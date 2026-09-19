#pragma once

#include <iostream.h>

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
 * The body is not written yet.
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
 * The object is four bytes and the class is not polymorphic, so it emits no vtable. `Q23Mid3MBT`
 * is the one descriptor in the image whose demangled form matches what Print() produces, and the
 * accessor at `0x003d63f8` builds it through TypeInfo__ConstructBuiltin with no base list, which
 * is the shape a non-polymorphic type takes. Only that accessor refers to the descriptor, so the
 * match between the descriptor and the three routines below is inferred rather than proven.
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
 * 1000000000.0 and appends `s`, and this class writes the three-part form above. Two headers
 * elsewhere in this tree, `msg/catchprogresspacket.h` and `msg/trackselectpacket.h`, model a
 * member streamed through the emission at `0x004acf28` as a CmdID on the strength of the transfer
 * alone. The transfer cannot separate the three types.
 *
 * The bodies are not written yet. The three addresses below are the emission the scheduler
 * translation unit uses, and a second emission of Save() exists elsewhere, which is the shape of
 * an inline member rather than of an ordinary out-of-line one.
 *
 * The one member is public, because the readers of a position apply arithmetic to it directly and
 * the image exposes no accessor.
 */
class MBT {
public:
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
    void Print(ostream &stream);

    /**
     * The position, in MIDI ticks at 480 per quarter note.
     *
     * +0x00
     */
    int mTick;
};

} // namespace Mid
