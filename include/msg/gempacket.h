#pragma once

#include "msg/toallothergamesystemspacket.h"

/**
 * Network packet the game sends between game systems.
 *
 * `9GemPacket` in the RTTI descriptor at `0x008ef710`, with ToAllOtherGameSystemsPacket as its
 * one base. The object is 0x2c bytes and its vtable is at `0x00814380`. The payload comes from
 * the copy constructor at `0x003f3d18`, which Clone() delegates to, so the offsets and widths are
 * recovered but the purpose of each field is not. The four words Packet owns are declared there
 * rather than here.
 *
 * This class shares its RTTI accessor and vtable with ToAllOtherGameSystemsPacket, its own base,
 * which has no implementation of its own. The vtable belongs to this class.
 *
 * The class overrides Message::Print() at `0x003f2878`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class GemPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f1750
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nGemPacketType.
     * @ghidraAddress 0x003f17c8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `GemPacket`.
     * @ghidraAddress 0x003f17d8
     */
    virtual const char *Name();

private:
    long long mUnknown14; // +0x14
    long long mUnknown1c; // +0x1c
    int mUnknown24;       // +0x24
    int mUnknown28;       // +0x28
};

/**
 * Identity that GemPacket::Type() reports.
 *
 * This word belongs to GemPacket because GemPacket::Type() at `0x003f17c8` returns it.
 *
 * @ghidraAddress 0x006d73cc
 */
extern int g_nGemPacketType;
