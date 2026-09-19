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
 * LevelConverter is the one subclass in the image, and it overrides slots 2 through 10. That is
 * also the whole of the evidence about these signatures. Slots 3 through 7 are certain, because
 * each override forwards its arguments to LevelBuilder with a MIDI status byte, 0x90, 0x80, 0xb0,
 * 0xc0, and 0xe0 in slot order, and those five bytes are note on, note off, controller, program
 * change, and pitch bend. Slots 2, 8, 9, and 10 are inferred from what the override does rather
 * than from a status byte.
 *
 * Slots 11 through 15 have no override anywhere, so an empty body is the whole of what the image
 * records. Their names, parameter lists, and return types are all unrecovered, and each is
 * declared below with its index because a gap in the sequence would describe a different class.
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
     * Report that the whole file has been delivered.
     *
     * Slot 8. LevelConverter's override closes the builder and sets its own finished flag, which is
     * what the name comes from. Its position ahead of TextEvent() is the declaration order the
     * table records.
     *
     * The arity comes from the override rather than from the default. LevelConverter's override at
     * `0x001ea570` sets only the object register before calling LevelBuilder::Finish(), and
     * Finish() reads its own third register, so the second and third registers pass straight
     * through from this slot's parameter list.
     *
     * @param nUnknown The first parameter, which LevelBuilder::Finish() does not read.
     * @param pUnknown The second parameter, which LevelBuilder::Finish() constructs from.
     * @ghidraAddress 0x001ea2a8
     */
    virtual void AllDone(int nUnknown, void *pUnknown);

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
     * Unrecovered. Slot 11, and an empty default.
     *
     * LevelConverter's table points at a two-instruction body of its own at `0x001ea360`, which is
     * this default re-emitted rather than an override. The parameter list and return type are both
     * unrecovered.
     *
     * @ghidraAddress 0x001ea2c0
     */
    virtual void OnUnknownSlot11();

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
