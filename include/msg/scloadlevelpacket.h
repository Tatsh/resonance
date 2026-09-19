#pragma once

#include "game/gameparams.h"
#include "msg/toallgamecontrollerspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `17SCLoadLevelPacket` in the RTTI descriptor at `0x008ef610`, with ToAllGameControllersPacket
 * as its one base. The object is 0x4c bytes and its vtable is at `0x00814410`. The payload comes
 * from the copy constructor at `0x003f3c48`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * The class overrides Message::Print() at `0x003f2858`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SCLoadLevelPacket : public ToAllGameControllersPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f13a0
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCLoadLevelPacketType.
     * @ghidraAddress 0x003f1418
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCLoadLevelPacket`.
     * @ghidraAddress 0x003f1428
     */
    virtual const char *Name();

private:
    GameParams mUnknown14; // +0x14
};

/**
 * Identity that SCLoadLevelPacket::Type() reports.
 *
 * This word belongs to SCLoadLevelPacket because SCLoadLevelPacket::Type() at `0x003f1418`
 * returns it.
 *
 * @ghidraAddress 0x006d73bc
 */
extern int g_nSCLoadLevelPacketType;
