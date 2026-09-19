#pragma once

#include <vector>

#include "msg/toallothernetmanagerspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `24SCAllClientsStatusPacket` in the RTTI descriptor at `0x008efe40`, with
 * ToAllOtherNetManagersPacket as its one base. The object is 0x20 bytes and its vtable is at
 * `0x00814770`. The payload comes from the copy constructor at `0x003f3298`, which Clone()
 * delegates to, and it accounts for the allocation exactly. The four words Packet owns are
 * declared there rather than here.
 *
 * The vector at `+0x14` is deep-copied, so the class owns its elements. The element stride is 0x8
 * bytes, taken from the divide the copy uses to count them, but the element type is not
 * recovered, so it is declared as a record of that size.
 *
 * The class overrides Message::Print() at `0x003f2160`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SCAllClientsStatusPacket : public ToAllOtherNetManagersPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003efaa8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCAllClientsStatusPacketType.
     * @ghidraAddress 0x003efb20
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCAllClientsStatusPacket`.
     * @ghidraAddress 0x003efb30
     */
    virtual const char *Name();

private:
    // Element of the vector at +0x14. Only its size is recovered.
    struct Entry14 {
        int mUnknown00;
        int mUnknown04;
    };

    std::vector<Entry14> mUnknown14; // +0x14
};

/**
 * Identity that SCAllClientsStatusPacket::Type() reports.
 *
 * This word belongs to SCAllClientsStatusPacket because SCAllClientsStatusPacket::Type() at
 * `0x003efb20` returns it.
 *
 * @ghidraAddress 0x006d7394
 */
extern int g_nSCAllClientsStatusPacketType;
