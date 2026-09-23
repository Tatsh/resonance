#pragma once

namespace Mid {

/**
 * Sink a Standard MIDI File reader delivers one event at a time to.
 *
 * `Q23Mid8Receiver` in the RTTI descriptor at `0x0086f5f0`, a leaf with no base list. The class has
 * no data member, so the vptr sits at `+0x00` and the object is four bytes. Its table is at
 * `0x007e7740` and has sixteen entries with a zero terminator at index 16. Every one of the
 * fourteen virtuals below is a two-instruction `jr ra` default, so an unoverridden slot does
 * nothing.
 *
 * LevelConverter is the one subclass in the image, and it overrides slots 2 through 10.
 * Mid::FileReader is the one caller, and its call sites fix slots 2 through 11. Slots 3 through 7
 * each receive one channel event, and LevelConverter forwards each to LevelBuilder with a MIDI
 * status byte (0x90 note on, 0x80 note off, 0xb0 controller, 0xc0 program change, and 0xe0 pitch
 * bend, in slot order). The reader calls slot 8 for a tempo meta event, slot
 * 9 for a text meta event, slot 10 at the end of each track, and slot 11 at the end of the file.
 *
 * Slots 12 through 15 have no override and no caller anywhere. An empty body is the whole of what
 * the image records for each. Their names, parameter lists, and return types are all unrecovered,
 * and each is declared below with its index because a gap in the sequence would describe a
 * different class.
 *
 * Every method name here is inferred. RTTI in this image yields class names only.
 */
class Receiver {
public:
    /**
     * @ghidraAddress 0x001ea248
     */
    virtual ~Receiver();

    /**
     * Begin one track of the file.
     *
     * Slot 2.
     *
     * @param nTrack The track index.
     * @ghidraAddress 0x001ea278
     */
    virtual void NewTrack(unsigned char nTrack);

    /**
     * Receive a note on.
     *
     * Slot 3. LevelConverter's override forwards these arguments under MIDI status 0x90.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nNote The note number.
     * @param nVelocity The velocity.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea280
     */
    virtual void
    NoteOn(int nTick, unsigned char nNote, unsigned char nVelocity, unsigned char nChannel);

    /**
     * Receive a note off.
     *
     * Slot 4. LevelConverter's override forwards these arguments under MIDI status 0x80. The slot
     * takes no velocity, and the override's forwarded call passes zero in its place.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nNote The note number.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea288
     */
    virtual void NoteOff(int nTick, unsigned char nNote, unsigned char nChannel);

    /**
     * Receive a controller change.
     *
     * Slot 5. LevelConverter's override forwards these arguments under MIDI status 0xb0.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nController The controller number.
     * @param nValue The value.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea290
     */
    virtual void
    Controller(int nTick, unsigned char nController, unsigned char nValue, unsigned char nChannel);

    /**
     * Receive a program change.
     *
     * Slot 6. LevelConverter's override forwards these arguments under MIDI status 0xc0.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nProgram The program number.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea298
     */
    virtual void ProgramChange(int nTick, unsigned char nProgram, unsigned char nChannel);

    /**
     * Receive a pitch bend.
     *
     * Slot 7. LevelConverter's override forwards these arguments under MIDI status 0xe0.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nLow The low seven bits of the bend.
     * @param nHigh The high seven bits of the bend.
     * @param nChannel The channel.
     * @ghidraAddress 0x001ea2a0
     */
    virtual void
    PitchBend(int nTick, unsigned char nLow, unsigned char nHigh, unsigned char nChannel);

    /**
     * Receive a tempo meta event.
     *
     * Slot 8. Mid::FileReader::ReadMeta() calls it only for meta type 0x51, with the event
     * position and the event's 24-bit value. That value is the MIDI tempo in microseconds per
     * quarter note. LevelConverter's override passes both to LevelBuilder::SetTempo(). SetTempo()
     * builds its tempo map from the second.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param nMicrosecondsPerQuarter The tempo.
     * @ghidraAddress 0x001ea2a8
     */
    virtual void Tempo(int nTick, int nMicrosecondsPerQuarter);

    /**
     * Receive one text meta event.
     *
     * Slot 9. LevelConverter's override acts only on a type of 3 at tick 0, and 3 is the MIDI
     * track-name meta type, which is what fixes the third argument as the meta type.
     *
     * @param nTick The event position, in MIDI ticks.
     * @param pText The text, which is NUL-terminated by the reader.
     * @param nType The meta type.
     * @ghidraAddress 0x001ea2b0
     */
    virtual void TextEvent(int nTick, const char *pText, unsigned char nType);

    /**
     * End the track NewTrack() began.
     *
     * Slot 10.
     *
     * @ghidraAddress 0x001ea2b8
     */
    virtual void EndTrack();

    /**
     * Report that the whole file has been delivered.
     *
     * Slot 11. Mid::FileReader::EndOfFile() calls it with no argument once the chunk reader has
     * no chunk left. LevelConverter's table points at a two-instruction body of its own at
     * `0x001ea360`. That body is this default re-emitted rather than an override.
     *
     * @ghidraAddress 0x001ea2c0
     */
    virtual void AllDone();

    /**
     * Unrecovered. Slot 12, and an empty default with no override anywhere.
     *
     * @ghidraAddress 0x001ea2c8
     */
    virtual void OnUnknownSlot12();

    /**
     * Unrecovered. Slot 13, and an empty default with no override anywhere.
     *
     * @ghidraAddress 0x001ea2d0
     */
    virtual void OnUnknownSlot13();

    /**
     * Unrecovered. Slot 14, and an empty default with no override anywhere.
     *
     * @ghidraAddress 0x001ea2d8
     */
    virtual void OnUnknownSlot14();

    /**
     * Unrecovered. Slot 15, and an empty default with no override anywhere.
     *
     * @ghidraAddress 0x001ea2e0
     */
    virtual void OnUnknownSlot15();
};

} // namespace Mid
