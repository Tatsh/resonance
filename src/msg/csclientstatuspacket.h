#pragma once

#include <iostream>

#include "msg/tohostpacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * Its RTTI descriptor is at `0x008ef028`. It has ToHostPacket as its one base. The object is 0x18
 * bytes and its vtable is at `0x008147b8`. The payload comes from the copy constructor at
 * `0x003f3250`. Clone() delegates to it. The offsets and widths are recovered, but the purpose of
 * each field is not. The four words Packet provides are declared there rather than here.
 *
 * Its vtable has eight entries and a zero terminator at index 8. Slots 5, 6, and 7 are all its own
 * overrides. Both transfer members open by expanding the Packet pair inline rather than calling it,
 * which every one of the twenty overriding packet classes does identically.
 *
 * Both transfer members move the word Packet stores at `+0x0c` a second time, after this class's
 * own member. Six of the twenty do that, and each of the six gives the repeat its own stack slot
 * rather than reusing the one from the prefix, which is what establishes it as two expressions in
 * the original rather than one.
 *
 * The destructor at `0x003ef8f8` is compiler-generated and has no declaration here.
 */
class CSClientStatusPacket : public ToHostPacket {
public:
    /**
     * Produce a default-constructed packet on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nCSClientStatusPacketType. Only the Packet words are initialised.
     *
     * @return The packet.
     * @ghidraAddress NTSC-U/C: 0x003e4e00
     * @ghidraAddress PAL: 0x0041d098
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003ef970
     * @ghidraAddress PAL: 0x00427f78
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCSClientStatusPacketType.
     * @ghidraAddress NTSC-U/C: 0x003ef9e8
     * @ghidraAddress PAL: 0x00427ff0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CSClientStatusPacket`.
     * @ghidraAddress NTSC-U/C: 0x003ef9f8
     * @ghidraAddress PAL: 0x00428000
     */
    virtual const char *Name();

    /**
     * Write the payload to a diagnostic stream.
     *
     * Slot 5. The output is the literal `ClientStatus: ` at `0x008141c8` followed by the member.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003f2120
     * @ghidraAddress PAL: 0x0042a668
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the packet to a stream.
     *
     * Slot 6.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e6090
     * @ghidraAddress PAL: 0x0041e370
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the packet back from a stream.
     *
     * Slot 7. The member arrives in a local that construction clears before the transfer.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x003e6198
     * @ghidraAddress PAL: 0x0041e478
     */
    virtual void Load(IBStream &stream);

private:
    int mStatus; /*!< The client's status, labelled `ClientStatus: ` by Print(). +0x14 */
};

/**
 * Identity that CSClientStatusPacket::Type() reports.
 *
 * This word belongs to CSClientStatusPacket because CSClientStatusPacket::Type() at `0x003ef9e8`
 * returns it.
 *
 * @ghidraAddress NTSC-U/C: 0x006d738c
 * @ghidraAddress PAL: 0x0071ab2c
 */
extern int g_nCSClientStatusPacketType;
