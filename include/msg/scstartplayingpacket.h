#pragma once

#include "msg/toallgamecontrollerspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `20SCStartPlayingPacket` in the RTTI descriptor at `0x00902160`, with
 * ToAllGameControllersPacket as its one base. The object is 0x14 bytes and its vtable is at
 * `0x00814698`. The payload comes from the copy constructor at `0x003f3720`, which Clone()
 * delegates to, so the offsets and widths are recovered but the purpose of each field is not. The
 * four words Packet owns are declared there rather than here.
 *
 * The class adds no payload. Its allocation is exactly the 0x14 bytes Packet occupies, which is
 * what measures Packet.
 *
 * The class overrides Message::Print() at `0x003f02a0`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SCStartPlayingPacket : public ToAllGameControllersPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f01f8
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSCStartPlayingPacketType.
     * @ghidraAddress 0x003f0270
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SCStartPlayingPacket`.
     * @ghidraAddress 0x003f0280
     */
    virtual const char *Name();
};

/**
 * Identity that SCStartPlayingPacket::Type() reports.
 *
 * This word belongs to SCStartPlayingPacket because SCStartPlayingPacket::Type() at `0x003f0270`
 * returns it.
 *
 * @ghidraAddress 0x006d73ac
 */
extern int g_nSCStartPlayingPacketType;
