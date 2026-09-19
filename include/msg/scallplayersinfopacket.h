#pragma once

#include <vector>

#include "msg/toallothernetmanagerspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `22SCAllPlayersInfoPacket` in the RTTI descriptor at `0x008f2a80`, with
 * ToAllOtherNetManagersPacket as its one base. The object is 0x20 bytes and its vtable is at
 * `0x008146e0`. The payload comes from the copy constructor at `0x003f34e8`, which Clone()
 * delegates to, and it accounts for the allocation exactly. The four words Packet owns are
 * declared there rather than here.
 *
 * The vector at `+0x14` is deep-copied, so the class owns its elements. The element stride is
 * 0x14 bytes, taken from the divide the copy uses to count them, but the element type is not
 * recovered, so it is declared as a record of that size.
 *
 * The class overrides Message::Print() at `0x003f2278`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SCAllPlayersInfoPacket : public ToAllOtherNetManagersPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003eff60
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCAllPlayersInfoPacketType.
     * @ghidraAddress 0x003effd8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCAllPlayersInfoPacket`.
     * @ghidraAddress 0x003effe8
     */
    virtual const char *Name();

private:
    // Element of the vector at +0x14. Only its size is recovered.
    struct Entry14 {
        int mUnknown00;
        int mUnknown04;
        int mUnknown08;
        int mUnknown0c;
        int mUnknown10;
    };

    std::vector<Entry14> mUnknown14; // +0x14
};

/**
 * Identity that SCAllPlayersInfoPacket::Type() reports.
 *
 * This word belongs to SCAllPlayersInfoPacket because SCAllPlayersInfoPacket::Type() at
 * `0x003effd8` returns it.
 *
 * @ghidraAddress 0x006d73a4
 */
extern int g_nSCAllPlayersInfoPacketType;
