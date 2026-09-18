#pragma once

#include "msg/tohostpacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `20CSClientStatusPacket` in the RTTI descriptor at `0x008ef028`, with ToHostPacket as its one
 * base. The object is 0x18 bytes and its vtable is at `0x008147b8`. The payload comes from the
 * copy constructor at `0x003f3250`, which Clone() delegates to, so the offsets and widths are
 * recovered but the purpose of each field is not. The four words Packet owns are declared there
 * rather than here.
 *
 * The class overrides Message::Print() at `0x003f2120`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class CSClientStatusPacket : public ToHostPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003ef970
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCSClientStatusPacketType.
     * @ghidraAddress 0x003ef9e8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CSClientStatusPacket`.
     * @ghidraAddress 0x003ef9f8
     */
    virtual const char *Name();

private:
    int mUnknown14; // +0x14
};

/**
 * Identity that CSClientStatusPacket::Type() reports.
 *
 * This word belongs to CSClientStatusPacket because CSClientStatusPacket::Type() at `0x003ef9e8`
 * returns it.
 *
 * @ghidraAddress 0x006d738c
 */
extern int g_nCSClientStatusPacketType;
