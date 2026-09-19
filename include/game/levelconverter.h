#pragma once

#include <vector>

#include "mid/receiver.h"
#include "os/hxstr.h"

class Application;
class LevelBuilder;

/**
 * Converter that reads a Standard MIDI File and fills a LevelBuilder from its events.
 *
 * `14LevelConverter` in the RTTI descriptor at `0x008ef230`, with Mid::Receiver as its one base at
 * offset 0. Its table is at `0x007e76b8` and has sixteen entries with a zero terminator at index
 * 16, the same length as the base table, so the class adds no virtual. It overrides slots 2
 * through 10 and inherits slots 11 through 15.
 *
 * Slot 11 is worth stating separately. The derived table records `0x001ea360` where the base
 * records `0x001ea2c0`, and both are the same two instructions. This toolchain re-emits an inline
 * empty body into every translation unit that needs one, so a unique address for an empty body is
 * no evidence of an override. The slot is recorded as inherited and nothing is declared for it.
 *
 * The object is at least 0xcc bytes and much of it is unrecovered. What the constructor, the
 * destructor, Convert(), and NewTrack() establish is written below; every member whose purpose is
 * undetermined retains its offset.
 *
 * Convert() is the entry point. It stores the builder, extracts the file's base name into a global
 * buffer at `0x00891a38`, reads two display-configuration codes, builds three stack objects for
 * the file, the byte stream, and the Standard MIDI File reader, hands itself to the reader as the
 * event sink, and runs it. The reader is built at `0x003d4a68` and run at `0x003d6528`, and the
 * routine at `0x001e6450` is installed into the reader at its own `+0x30` as a callback.
 *
 * The five event handlers all follow one shape. They report an error through the reporter at
 * `0x001ea6e0` when either error flag is set, call the per-channel routine at `0x001e8120`, and
 * then forward to one of two sinks. With the flag at `+0x40` clear the event goes to
 * LevelBuilder::AddEvent() on the builder at `+0x50`; with it set the event goes to the routine at
 * `0x001ce8d0` on the object at `+0x54` instead, with the position rebased against `+0x5c` and
 * saturated to Mid::MBT's bounds. The two paths are what the flag at `+0x40` selects between.
 *
 * Eleven bodies here are not written yet. Every one is blocked on a routine outside this class
 * that has no recovered name: the file, stream, and reader classes at `0x00405cf8`, `0x00145c40`,
 * and `0x003d4a68` for Convert(); the per-channel routine at `0x001e8120`, the error reporter at
 * `0x001ea6e0`, and the second sink at `0x001ce8d0` for the five event handlers; the vector
 * assignment at `0x001e9440` for NewTrack(); and the name-map forwarders on LevelBuilder for
 * EndTrack().
 *
 * Every method name below that is not a Mid::Receiver override is inferred from its body.
 */
class LevelConverter : public Mid::Receiver {
public:
    /**
     * Eight-byte record the collection at `+0x70` stores.
     *
     * `PendingEvent` is a placeholder for the name. The size comes from the stride the destructor
     * and NewTrack() step the collection by. NewTrack() empties it and EndTrack() reports an error
     * when it is not empty by the end of a track, which is what makes it a set of events still
     * waiting for their partner.
     */
    struct PendingEvent {
        int mUnknown00; // +0x00
        int mUnknown04; // +0x04
    };

    /**
     * Twelve-byte record the three collections from `+0x98` to `+0xbb` store.
     *
     * `Span` is a placeholder for the name. The size comes from the stride the destructor steps
     * each of the three by, and from the division by twelve it uses to recover each count.
     */
    struct Span {
        int mUnknown00; // +0x00
        int mUnknown04; // +0x04
        int mUnknown08; // +0x08
    };

    /**
     * @ghidraAddress 0x001e6278
     */
    LevelConverter();

    /**
     * @ghidraAddress 0x001e9ee0
     */
    virtual ~LevelConverter();

    /**
     * Read one Standard MIDI File and fill a builder from it.
     *
     * The body is not written yet.
     *
     * @param pszPath The file to read.
     * @param nUnknown2 The third argument, forwarded to the file object at `0x00405cf8`.
     * @param nUnknown3 The fourth argument, forwarded to the same object.
     * @param pBuilder The builder the events are appended to.
     * @ghidraAddress 0x001e65e0
     */
    void Convert(const char *pszPath, int nUnknown2, int nUnknown3, LevelBuilder *pBuilder);

    /**
     * Reset the per-track state and begin one track.
     *
     * The body is not written yet. It stores the track index at `+0x10`, clears the four words
     * from `+0x3c` to `+0x48`, empties the pending-event collection at `+0x70`, sets `+0x14` and
     * `+0x64` to 0xff, clears `+0x0c`, `+0x54`, `+0x58`, and `+0x6c`, sets `+0x5c`, `+0x88`, and
     * `+0xc4` to -1, and replaces the name map at `+0x7c` with an empty one through the vector
     * assignment at `0x001e9440`.
     *
     * @param nTrack The track index.
     * @ghidraAddress 0x001e6880
     */
    virtual void NewTrack(unsigned char nTrack);

    /**
     * Receive a note on and forward it under MIDI status 0x90.
     *
     * The body is not written yet. Beyond the shared shape, a set flag at `+0x44` sends the event
     * to the routine at `0x001e7ac0` instead of to either sink, and one path reports an error
     * through the reporter with the literal at `0x007e7268`.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nNote The note number.
     * @param nVelocity The velocity.
     * @param nChannel The channel.
     * @ghidraAddress 0x001e6fc0
     */
    virtual void
    NoteOn(int nTick, unsigned char nNote, unsigned char nVelocity, unsigned char nChannel);

    /**
     * Receive a note off and forward it under MIDI status 0x80.
     *
     * The body is not written yet. The forwarded call passes zero where the note-on path passes a
     * velocity, because this slot receives none.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nNote The note number.
     * @param nChannel The channel.
     * @ghidraAddress 0x001e7188
     */
    virtual void NoteOff(int nTick, unsigned char nNote, unsigned char nChannel);

    /**
     * Receive a controller change and forward it under MIDI status 0xb0.
     *
     * The body is not written yet. It has eight separate calls to the error reporter, more than
     * any other handler, so most of its length is validation of the controller number against the
     * state the track has reached.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nController The controller number.
     * @param nValue The value.
     * @param nChannel The channel.
     * @ghidraAddress 0x001e7788
     */
    virtual void
    Controller(int nTick, unsigned char nController, unsigned char nValue, unsigned char nChannel);

    /**
     * Receive a program change and forward it under MIDI status 0xc0.
     *
     * The body is not written yet. With the flag at `+0x40` clear it stores the program at `+0x64`
     * and sends nothing, which is the one handler that withholds an event rather than forwarding
     * it to the second sink.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nProgram The program number.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea370
     */
    virtual void ProgramChange(int nTick, unsigned char nProgram, unsigned char nChannel);

    /**
     * Receive a pitch bend and forward it under MIDI status 0xe0.
     *
     * The body is not written yet. On the second-sink path it first calls the routine at
     * `0x001ea618`, reports an error when `+0x54` is null, then rebases the position against
     * `+0x5c` and saturates the difference to Mid::MBT's bounds before forwarding.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nLow The low seven bits of the bend.
     * @param nHigh The high seven bits of the bend.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea420
     */
    virtual void
    PitchBend(int nTick, unsigned char nLow, unsigned char nHigh, unsigned char nChannel);

    /**
     * Close the builder and mark the conversion finished.
     *
     * Both parameters pass through to LevelBuilder::Finish() in the registers they arrive in, so
     * neither appears as a named use in the body.
     *
     * @param nUnknown The first parameter, forwarded.
     * @param pUnknown The second parameter, forwarded.
     * @ghidraAddress 0x001ea570
     */
    virtual void AllDone(int nUnknown, void *pUnknown);

    /**
     * Parse a track name, and ignore every other text event.
     *
     * A type other than 3 and a position other than 0 are both ignored, and 3 is the MIDI
     * track-name meta type. The discarded IsFiniteMBT(0) ahead of the test is the shape of an
     * assertion compiled without its report.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param pText The text.
     * @param nType The meta type.
     * @ghidraAddress 0x001ea5a0
     */
    virtual void TextEvent(int nTick, const char *pText, unsigned char nType);

    /**
     * Finish the track NewTrack() began.
     *
     * The body is not written yet. A recovered `+0x88` hands the name map to the builder through
     * the forwarder at `0x001ec560`. A set flag at `+0x40` with `+0x6c` clear calls the second
     * forwarder at `0x001ec580` with two zeroes. A `+0x54` present with `+0x0c` equal to 2 and the
     * word at `+0x54` plus 0x18 clear reports an error with the literal at `0x007e6fd0`. A
     * pending-event collection that is not empty reports an error with the literal at
     * `0x007e6ff8`, and an empty one instead empties the three span collections.
     *
     * @ghidraAddress 0x001e6a30
     */
    virtual void EndTrack();

private:
    /**
     * Read a track name and set the track's kind from it.
     *
     * The body is not written yet. It is 463 instructions and the largest routine of the class,
     * and its one caller is TextEvent().
     *
     * @param pText The track name.
     * @ghidraAddress 0x001e8318
     */
    void ParseTrackTypeString(const char *pText);

    HxStr mPath;              // +0x04, assigned from Convert's path argument
    int mUnknown0c;           // +0x0c, compared against 2 by EndTrack
    int mTrack;               // +0x10, the track index, set to -1 by Convert
    unsigned char mUnknown14; // +0x14, set to 0xff by NewTrack
    HxStr mUnknown18;         // +0x18
    HxStr mUnknown24;         // +0x24
    int mUnknown2c;           // +0x2c, cleared by Convert
    int mUnknown30;           // +0x30, cleared by Convert
    int mUnknown34;           // +0x34, cleared by Convert
    int mUnknown38;           // +0x38, cleared by Convert
    int mUnknown3c;           // +0x3c, cleared by NewTrack
    // Selects which sink an event goes to. Clear sends it to the builder at +0x50, set sends it to
    // the object at +0x54.
    int mUnknown40; // +0x40
    // Either error flag set makes every event handler report and return.
    int mUnknown44;         // +0x44
    int mUnknown48;         // +0x48
    int mUnknown4c;         // +0x4c
    LevelBuilder *mBuilder; // +0x50
    // The second sink, whose appender is at 0x001ce8d0. Its class is unrecovered.
    void *mUnknown54; // +0x54
    int mUnknown58;   // +0x58
    // The position the second sink's events are rebased against. Mid::kMBTInfinity at
    // construction, and -1 at the start of each track.
    int mUnknown5c; // +0x5c
    int mUnknown60; // +0x60, Mid::kMBTInfinity at construction
    int mUnknown64; // +0x64, the withheld program number, 0xff at the start of each track
    int mUnknown68; // +0x68
    int mUnknown6c; // +0x6c, cleared by NewTrack
    std::vector<PendingEvent> mPending; // +0x70
    // Element size 1, from the byte arithmetic in the vector assignment at 0x001e9440 and in the
    // destructor. EndTrack hands it to the builder.
    std::vector<char> mNameMap; // +0x7c
    int mUnknown88; // +0x88, Mid::kMBTInfinity at construction, -1 at the start of each track
    int mFinished;  // +0x8c, cleared by Convert and set to 1 by AllDone
    int mUnknown90; // +0x90
    int mUnknown94; // +0x94
    std::vector<Span> mUnknown98; // +0x98
    std::vector<Span> mUnknowna4; // +0xa4
    std::vector<Span> mUnknownb0; // +0xb0
    int mUnknownbc;               // +0xbc
    // Set by Convert from configuration code 0x3a4, and to zero when code 0x3a1 reports non-zero.
    int mUnknownc0; // +0xc0
    int mUnknownc4; // +0xc4, set to -1 by NewTrack
    // Set by Convert from the accessor at 0x00118dd8 applied to Application::shared().
    void *mUnknownc8; // +0xc8
};
