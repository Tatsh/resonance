#pragma once

#include "game/freqappearance.h"
#include "msg/tohostpacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `19PSJoinRequestPacket` in the RTTI descriptor at `0x008ef0b0`, with ToHostPacket as its one
 * base. The object is 0x28 bytes and its vtable is at `0x008148d8`. The payload comes from the
 * copy constructor at `0x003f2dc0`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * Its vtable has eight entries and a zero terminator at index 8. Slots 6 and 7 are its own
 * overrides rather than the inherited Packet ones at `0x003f1de8` and `0x003f1ea0`. Neither
 * override calls the base. Each repeats the four-word transfer inline and adds the appearance.
 *
 * The destructor at `0x003eee18` is compiler-generated and has no declaration here.
 */
class PSJoinRequestPacket : public ToHostPacket {
public:
    /**
     * Construct a packet with a fresh appearance.
     *
     * The image lists no caller for the out-of-line body. New() expands the same stores in place.
     *
     * @ghidraAddress 0x003eef50
     */
    PSJoinRequestPacket();

    /**
     * Produce a packet with a fresh appearance on the heap.
     *
     * The registry the translation unit at `0x003ed2e0` builds stores this address against
     * g_nPSJoinRequestPacketType.
     *
     * @return The packet.
     * @ghidraAddress 0x003e4a98
     */
    static Message *New();

    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003eeeb8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nPSJoinRequestPacketType.
     * @ghidraAddress 0x003eef30
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `PSJoinRequestPacket`.
     * @ghidraAddress 0x003eef40
     */
    virtual const char *Name();

    /**
     * Write the packet to a stream.
     *
     * Slot 6. The word Packet stores at `+0x0c` is written twice, once in the four-word prefix and
     * again after the appearance.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e5538
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the packet back from a stream.
     *
     * Slot 7. Mirrors Save(), including the repeated transfer of `+0x0c`.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e5638
     */
    virtual void Load(IBStream &stream);

    /**
     * Write the appearance to a diagnostic stream through FreqAppearance::Print().
     *
     * Slot 5.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f1f38
     */
    virtual void Print(std::ostream &stream);

private:
    FreqAppearance mUnknown14; // +0x14
};

/**
 * Identity that PSJoinRequestPacket::Type() reports.
 *
 * This word belongs to PSJoinRequestPacket because PSJoinRequestPacket::Type() at `0x003eef30`
 * returns it.
 *
 * @ghidraAddress 0x006d7368
 */
extern int g_nPSJoinRequestPacketType;
