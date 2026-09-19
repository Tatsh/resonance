#pragma once

#include "msg/message.h"

/**
 * Abstract base of every network packet.
 *
 * `6Packet` in the RTTI descriptor at `0x009021a0`, with Message as its one base. Its vtable at
 * `0x00814920` has eight entries and a zero terminator at index 8. Slots 2, 3, and 4 address the
 * pure-virtual handler, so the class implements none of Clone(), Type(), or Name() and is never
 * instantiated. Slot 5 retains Message::Print(). Slots 6 and 7 are the class's own overrides.
 *
 * Slot 0 of that table constructs the descriptor above from the mangled name `6Packet` at
 * `0x00814aa0`, which is what establishes the table as this class's rather than as a derived
 * class's. Both overrides were titled against CSInitiatePlayPacket, whose own table at
 * `0x00814728` records the same two addresses because that class adds no payload and inherits the
 * pair. The 22 concrete packet classes all inherit both.
 *
 * Both members are very likely inline in this header in the original, and the definitions in
 * `packet.cpp` are the out-of-line emission that fills the table slots. Every one of the twenty
 * overriding packet classes opens its own Save() with a byte-identical expansion of the four
 * transfers below, and not one of them contains a call instruction to either address. A qualified
 * call to a non-inline out-of-line member would compile to a call, so the body was available for
 * expansion. That is the same evidence this tree already accepts for the inlined HxStr destructor.
 * Moving the two definitions into this header is a deliberate one-time change rather than something
 * to discover partway through the derived classes, so the question is recorded here instead.
 *
 * The four words below transfer through those two overrides, which recovers the layout from the
 * class's own code. Three unrelated routing families give the identical shared prefix
 * independently, and two packets, CSInitiatePlayPacket and SCStartPlayingPacket, are exactly 0x14
 * bytes and add nothing to it.
 */
class Packet : public Message {
public:
    /**
     * Write the four words to a stream.
     *
     * Slot 6.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f1de8
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the four words back from a stream.
     *
     * Slot 7. The words come back in the order Save() wrote them, and each transfer fills its
     * field in place.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003f1ea0
     */
    virtual void Load(IBStream &stream);

protected:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
};
