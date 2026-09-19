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
 * The class overrides Message::Print() at `0x003f1f38`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class PSJoinRequestPacket : public ToHostPacket {
public:
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
