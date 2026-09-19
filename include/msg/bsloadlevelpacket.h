#pragma once

#include "game/gameparams.h"
#include "msg/tohostpacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `17BSLoadLevelPacket` in the RTTI descriptor at `0x008ef600`, with ToHostPacket as its one
 * base. The object is 0x4c bytes and its vtable is at `0x00814458`. The payload comes from the
 * copy constructor at `0x003f3bc0`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * The class overrides Message::Print() at `0x003f2780`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class BSLoadLevelPacket : public ToHostPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f1118
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nBSLoadLevelPacketType.
     * @ghidraAddress 0x003f1190
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `BSLoadLevelPacket`.
     * @ghidraAddress 0x003f11a0
     */
    virtual const char *Name();

private:
    GameParams mUnknown14; // +0x14
};

/**
 * Identity that BSLoadLevelPacket::Type() reports.
 *
 * This word belongs to BSLoadLevelPacket because BSLoadLevelPacket::Type() at `0x003f1190`
 * returns it.
 *
 * @ghidraAddress 0x006d73b4
 */
extern int g_nBSLoadLevelPacketType;
