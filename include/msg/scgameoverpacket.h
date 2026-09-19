#pragma once

#include "msg/toallgamecontrollerspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `16SCGameOverPacket` in the RTTI descriptor at `0x008efcd0`, with ToAllGameControllersPacket as
 * its one base. The object is 0x18 bytes and its vtable is at `0x008143c8`. The payload comes
 * from the copy constructor at `0x003f3cd0`, which Clone() delegates to, so the offsets and
 * widths are recovered but the purpose of each field is not. The four words Packet owns are
 * declared there rather than here.
 *
 * This class shares its RTTI accessor and vtable with ToAllGameControllersPacket, its own base,
 * which has no implementation of its own. The vtable belongs to this class.
 *
 * Its vtable has eight entries and a zero terminator at index 8. Slots 5, 6, and 7 are all its own
 * overrides rather than the inherited ones. Both transfer members open by expanding the Packet pair
 * inline rather than calling it, which every one of the twenty overriding packet classes does
 * identically.
 */
class SCGameOverPacket : public ToAllGameControllersPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f15d0
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCGameOverPacketType.
     * @ghidraAddress 0x003f1648
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCGameOverPacket`.
     * @ghidraAddress 0x003f1658
     */
    virtual const char *Name();

    /**
     * Write the payload to a diagnostic stream.
     *
     * Slot 5. The one member is the whole output, with no literal around it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2a90
     */
    virtual void Print(ostream &stream);

    /**
     * Write the packet to a stream.
     *
     * Slot 6. The member reaches the stream as a single byte rather than as a word.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2920
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the packet back from a stream.
     *
     * Slot 7.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003f29e8
     */
    virtual void Load(IBStream &stream);

private:
    int mUnknown14; // +0x14
};

/**
 * Identity that SCGameOverPacket::Type() reports.
 *
 * This word belongs to SCGameOverPacket because SCGameOverPacket::Type() at `0x003f1648` returns
 * it.
 *
 * @ghidraAddress 0x006d73c4
 */
extern int g_nSCGameOverPacketType;
