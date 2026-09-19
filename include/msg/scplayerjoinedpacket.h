#pragma once

#include "game/playerinfo.h"
#include "msg/toallnetmanagerspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `20SCPlayerJoinedPacket` in the RTTI descriptor at `0x00902050`, with ToAllNetManagersPacket as
 * its one base. The object is 0x54 bytes and its vtable is at `0x00814800`. The payload comes
 * from the copy constructor at `0x003f31c8`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * The class overrides Message::Print() at `0x003f2100`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SCPlayerJoinedPacket : public ToAllNetManagersPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003ef718
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCPlayerJoinedPacketType.
     * @ghidraAddress 0x003ef790
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCPlayerJoinedPacket`.
     * @ghidraAddress 0x003ef7a0
     */
    virtual const char *Name();

private:
    PlayerInfo mUnknown14; // +0x14
};

/**
 * Identity that SCPlayerJoinedPacket::Type() reports.
 *
 * This word belongs to SCPlayerJoinedPacket because SCPlayerJoinedPacket::Type() at `0x003ef790`
 * returns it.
 *
 * @ghidraAddress 0x006d7384
 */
extern int g_nSCPlayerJoinedPacketType;
