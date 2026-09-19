#pragma once

#include <vector>

#include "msg/toallothergamesystemspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `13CripplePacket` in the RTTI descriptor at `0x008f0940`, with ToAllOtherGameSystemsPacket as
 * its one base. The object is 0x28 bytes and its vtable is at `0x008144e8`. The payload comes
 * from the copy constructor at `0x003f3938`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * The vector at `+0x1c` is deep-copied, so the class owns its elements. The element stride is 0x8
 * bytes, taken from the divide the copy uses to count them, but the element type is not
 * recovered, so it is declared as a record of that size.
 *
 * The class overrides Message::Print() at `0x003e7c38`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class CripplePacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0c60
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCripplePacketType.
     * @ghidraAddress 0x003f0cd8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CripplePacket`.
     * @ghidraAddress 0x003f0ce8
     */
    virtual const char *Name();

private:
    // Element of the vector at +0x1c. Only its size is recovered.
    struct Entry1c {
        int mUnknown00;
        int mUnknown04;
    };

    long long mUnknown14;            // +0x14
    std::vector<Entry1c> mUnknown1c; // +0x1c
};

/**
 * Identity that CripplePacket::Type() reports.
 *
 * This word belongs to CripplePacket because CripplePacket::Type() at `0x003f0cd8` returns it.
 *
 * @ghidraAddress 0x006d73fc
 */
extern int g_nCripplePacketType;
