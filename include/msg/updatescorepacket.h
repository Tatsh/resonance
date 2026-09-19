#pragma once

#include <iostream>

#include "msg/toallothergamesystemspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `17UpdateScorePacket` in the RTTI descriptor at `0x008efcc0`, with ToAllOtherGameSystemsPacket
 * as its one base. The object is 0x1c bytes and its vtable is at `0x008145c0`. The payload comes
 * from the copy constructor at `0x003f3818`, which Clone() delegates to, so the offsets and
 * widths are recovered but the purpose of each field is not. The four words Packet owns are
 * declared there rather than here.
 *
 * Its vtable has eight entries and a zero terminator at index 8. Slots 5, 6, and 7 are all its own
 * overrides. Both transfer members open by expanding the Packet pair inline rather than calling it,
 * which every one of the twenty overriding packet classes does identically.
 *
 * Print() labels the two members `pid:` and ` score-delta:`, so the first is a player identifier
 * and the second a score change. Both labels are display text rather than identifiers, and one of
 * them is not a valid identifier at all, so the members retain their recovered-purpose-pending
 * spelling.
 */
class UpdateScorePacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f06c0
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nUpdateScorePacketType.
     * @ghidraAddress 0x003f0738
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `UpdateScorePacket`.
     * @ghidraAddress 0x003f0748
     */
    virtual const char *Name();

    /**
     * Write the payload to a diagnostic stream.
     *
     * Slot 5. The literals are at `0x00814240` and `0x00814248`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003f2510
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the packet to a stream.
     *
     * Slot 6.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e7018
     */
    virtual void Save(OBStream &stream);

    /**
     * Read the packet back from a stream.
     *
     * Slot 7. Both members are filled in place rather than through a local.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x003e7120
     */
    virtual void Load(IBStream &stream);

private:
    int mUnknown14; // +0x14
    int mUnknown18; // +0x18
};

/**
 * Identity that UpdateScorePacket::Type() reports.
 *
 * This word belongs to UpdateScorePacket because UpdateScorePacket::Type() at `0x003f0738`
 * returns it.
 *
 * @ghidraAddress 0x006d73e4
 */
extern int g_nUpdateScorePacketType;
