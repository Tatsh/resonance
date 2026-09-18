#pragma once

#include "msg/toallothergamesystemspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `10BumpPacket` in the RTTI descriptor at `0x008ef700`, with ToAllOtherGameSystemsPacket as its
 * one base. The object is 0x28 bytes and its vtable is at `0x008144a0`. The payload comes from
 * the copy constructor at `0x003f3b58`, which Clone() delegates to, so the offsets and widths are
 * recovered but the purpose of each field is not. The four words Packet owns are declared there
 * rather than here.
 *
 * The class overrides Message::Print() at `0x003f26d8`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class BumpPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0e80
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nBumpPacketType.
     * @ghidraAddress 0x003f0ef8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `BumpPacket`.
     * @ghidraAddress 0x003f0f08
     */
    virtual const char *Name();

private:
    long long mUnknown14; // +0x14
    int mUnknown1c;       // +0x1c
    int mUnknown20;       // +0x20
    int mUnknown24;       // +0x24
};

/**
 * Identity that BumpPacket::Type() reports.
 *
 * This word belongs to BumpPacket because BumpPacket::Type() at `0x003f0ef8` returns it.
 *
 * @ghidraAddress 0x006d7404
 */
extern int g_nBumpPacketType;
