#pragma once

#include <iostream>

#include "msg/toallgamecontrollerspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008efcd0`. It has ToAllGameControllersPacket as its one base. The
 * object is 0x18 bytes and its vtable is at `0x008143c8`. The payload comes from the copy
 * constructor at `0x003f3cd0`. Clone() delegates to it. The offsets and widths are recovered, but
 * the purpose of each field is not. The four words Packet provides are declared there rather than
 * here.
 *
 * This class shares its RTTI accessor and vtable with ToAllGameControllersPacket, its own base,
 * which has no implementation of its own. The vtable belongs to this class.
 *
 * Its vtable has eight entries and a zero terminator at index 8. Slots 5, 6, and 7 are all its own
 * overrides rather than the inherited ones. Both transfer members open by expanding the Packet pair
 * inline rather than calling it, which every one of the twenty overriding packet classes does
 * identically.
 *
 * The destructor at `0x003f1558` is compiler-generated and has no declaration here.
 */
class SCGameOverPacket : public ToAllGameControllersPacket {
public:
    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nSCGameOverPacketType. Only the Packet words are initialised.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e50f0
     * @ghidraAddress PAL: 0x0041d388
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003f15d0
     * @ghidraAddress PAL: 0x00429a98
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCGameOverPacketType.
     * @ghidraAddress NTSC-U/C: 0x003f1648
     * @ghidraAddress PAL: 0x00429b10
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCGameOverPacket`.
     * @ghidraAddress NTSC-U/C: 0x003f1658
     * @ghidraAddress PAL: 0x00429b20
     */
    virtual const char *GetName() const;

    /**
     * Write the payload to a diagnostic stream.
     *
     * Slot 5. The one member is the whole output, with no literal around it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2a90
     * @ghidraAddress PAL: 0x0042afd8
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Write the packet to a stream.
     *
     * Slot 6. The member reaches the stream as a single byte rather than as a word.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2920
     * @ghidraAddress PAL: 0x0042ae68
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the packet back from a stream.
     *
     * Slot 7.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003f29e8
     * @ghidraAddress PAL: 0x0042af30
     */
    virtual void restoreGuts(IBStream &stream);

private:
    int mResult; /*!< The outcome of the game. The title is inferred from the class name. +0x14 */
};

/**
 * Identity that SCGameOverPacket::Type() reports.
 *
 * This word belongs to SCGameOverPacket because SCGameOverPacket::Type() at `0x003f1648` returns
 * it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d73c4
 * @ghidraAddress PAL: 0x0071ab64
 */
extern int g_nSCGameOverPacketType;
