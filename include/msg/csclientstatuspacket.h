#pragma once

#include "msg/tohostpacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `20CSClientStatusPacket` in the RTTI descriptor at `0x008ef028`, with ToHostPacket as its one
 * base. The object is 0x18 bytes and its vtable is at `0x008147b8`. The payload comes from the
 * copy constructor at `0x003f3250`, which Clone() delegates to, so the offsets and widths are
 * recovered but the purpose of each field is not. The four words Packet owns are declared there
 * rather than here.
 *
 * Its vtable has eight entries and a zero terminator at index 8. Slots 5, 6, and 7 are all its own
 * overrides. Both transfer members open by expanding the Packet pair inline rather than calling it,
 * which every one of the twenty overriding packet classes does identically.
 *
 * Both transfer members move the word Packet stores at `+0x0c` a second time, after this class's
 * own member. Six of the twenty do that, and each of the six gives the repeat its own stack slot
 * rather than reusing the one from the prefix, which is what establishes it as two expressions in
 * the original rather than one.
 */
class CSClientStatusPacket : public ToHostPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003ef970
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCSClientStatusPacketType.
     * @ghidraAddress 0x003ef9e8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CSClientStatusPacket`.
     * @ghidraAddress 0x003ef9f8
     */
    virtual const char *Name();

    /**
     * Write the payload to a diagnostic stream.
     *
     * Slot 5. The output is the literal `ClientStatus: ` at `0x008141c8` followed by the member.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2120
     */
    virtual void Print(ostream &stream);

    /**
     * Write the packet to a stream.
     *
     * Slot 6.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e6090
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the packet back from a stream.
     *
     * Slot 7. The member arrives in a local that construction clears before the transfer.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e6198
     */
    virtual void Load(IBStream &stream);

private:
    int mUnknown14; // +0x14
};

/**
 * Identity that CSClientStatusPacket::Type() reports.
 *
 * This word belongs to CSClientStatusPacket because CSClientStatusPacket::Type() at `0x003ef9e8`
 * returns it.
 *
 * @ghidraAddress 0x006d738c
 */
extern int g_nCSClientStatusPacketType;
