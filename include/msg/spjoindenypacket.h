#pragma once

#include "msg/tosinglenetmanagerpacket.h"
#include "os/hxstr.h"

/**
 * Network packet the game sends between game systems.
 *
 * `16SPJoinDenyPacket` in the RTTI descriptor at `0x008efe30`, with ToSingleNetManagerPacket as
 * its one base. The object is 0x20 bytes and its vtable is at `0x00814848`. The payload comes
 * from the copy constructor at `0x003f3130`, which Clone() delegates to, so the offsets and
 * widths are recovered but the purpose of each field is not. The four words Packet owns are
 * declared there rather than here.
 *
 * This class shares its RTTI accessor and vtable with ToSingleNetManagerPacket, its own base,
 * which has no implementation of its own. The vtable belongs to this class.
 *
 * The class overrides Message::Print() at `0x003f2000`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SPJoinDenyPacket : public ToSingleNetManagerPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003ef558
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSPJoinDenyPacketType.
     * @ghidraAddress 0x003ef5d0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SPJoinDenyPacket`.
     * @ghidraAddress 0x003ef5e0
     */
    virtual const char *Name();

private:
    int mUnknown14;   // +0x14
    HxStr mUnknown18; // +0x18
};

/**
 * Identity that SPJoinDenyPacket::Type() reports.
 *
 * This word belongs to SPJoinDenyPacket because SPJoinDenyPacket::Type() at `0x003ef5d0` returns
 * it.
 *
 * @ghidraAddress 0x006d737c
 */
extern int g_nSPJoinDenyPacketType;
