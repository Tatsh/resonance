#pragma once

#include "msg/toallothernetmanagerspacket.h"
#include "os/hxstr.h"

/**
 * Network packet the game sends between game systems.
 *
 * `14GameChatPacket` in the RTTI descriptor at `0x008efd70`, with ToAllOtherNetManagersPacket as
 * its one base. The object is 0x24 bytes and its vtable is at `0x00814338`. The payload comes
 * from the copy constructor at `0x003f3d88`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * The class overrides Message::Print() at `0x003f28e8`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class GameChatPacket : public ToAllOtherNetManagersPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f1950
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nGameChatPacketType.
     * @ghidraAddress 0x003f19c8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `GameChatPacket`.
     * @ghidraAddress 0x003f19d8
     */
    virtual const char *Name();

private:
    HxStr mUnknown14; // +0x14
    HxStr mUnknown1c; // +0x1c
};

/**
 * Identity that GameChatPacket::Type() reports.
 *
 * This word belongs to GameChatPacket because GameChatPacket::Type() at `0x003f19c8` returns it.
 *
 * @ghidraAddress 0x006d740c
 */
extern int g_nGameChatPacketType;
