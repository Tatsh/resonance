#pragma once

#include "msg/message.h"

/**
 * Abstract base of every network packet.
 *
 * Its RTTI descriptor is at `0x009021a0`. It has Message as its one base. Its vtable at
 * `0x00814920` has eight entries and a zero terminator at index 8. Slots 2, 3, and 4 address the
 * pure-virtual handler, so the class implements none of Clone(), Type(), or Name() and is never
 * instantiated. Slot 5 retains Message::Print(). Slots 6 and 7 are the class's own overrides.
 *
 * Slot 0 of that table constructs the descriptor above from this class's type name at
 * `0x00814aa0`, which is what establishes the table as this class's rather than as a derived
 * class's. CSInitiatePlayPacket's table at `0x00814728` records the same two addresses because
 * CSInitiatePlayPacket adds no payload and inherits the pair. The 22 concrete packet classes all
 * inherit both.
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
 *
 * The destructor at `0x003eedc8` is compiler-generated and has no declaration here.
 */
class Packet : public Message {
public:
    /**
     * Write the four words to a stream.
     *
     * Slot 6.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f1de8
     * @ghidraAddress PAL: 0x0042a330
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the four words back from a stream.
     *
     * Slot 7. The words come back in the order Save() wrote them, and each transfer fills its
     * field in place.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003f1ea0
     * @ghidraAddress PAL: 0x0042a3e8
     */
    virtual void Load(IBStream &stream);

protected:
    /**
     * Set the two routing words and mark the remaining two as unset.
     *
     * No address attaches to the constructor on its own. Every one of the 21 New() factories at
     * `0x003e4a98` through `0x003e54d8` expands it in place, storing a pair of small constants at
     * `+0x04` and `+0x08` and -1 at both `+0x0c` and `+0x10`. The pair is fixed by the routing
     * class the packet derives from, which is why each routing class supplies it: ToHostPacket
     * passes 0 and 0, ToSingleNetManagerPacket 1 and 0, ToAllOtherNetManagersPacket 2 and 0,
     * ToAllOtherGameSystemsPacket 2 and 3, ToAllNetManagersPacket 3 and 0,
     * ToAllGameControllersPacket 3 and 2, and ToArbiterPacket 0 and 1. The first word therefore
     * selects which peers receive the packet and the second which subsystem on each peer, but no
     * literal in the image labels either one.
     *
     * @param nDestination The peers that receive the packet.
     * @param nDestinationSystem The subsystem on each receiving peer.
     */
    Packet(int nDestination, int nDestinationSystem)
        : mDestination(nDestination), mDestinationSystem(nDestinationSystem), mClientId(-1),
          mTargetClientId(-1) {
    }

    int mDestination;       /*!< The peers that receive the packet. +0x04 */
    int mDestinationSystem; /*!< The subsystem on each receiving peer. +0x08 */

    /**
     * The client the packet concerns, or -1 until set. +0x0c
     *
     * GemPacket::Print() labels it `clid:`, and six packets transfer it a second time after their
     * payload.
     */
    int mClientId;

    /**
     * A second client identifier, -1 until set. +0x10
     *
     * The image does not write or read it outside the transfers. The title is inferred from the
     * -1 default it shares with mClientId.
     */
    int mTargetClientId;
};
