#pragma once

#include "msg/toallothergamesystemspacket.h"
#include "sch/cmdid.h"

/**
 * Network packet the game sends between game systems.
 *
 * `19CatchProgressPacket` in the RTTI descriptor at `0x00902060`, with
 * ToAllOtherGameSystemsPacket as its one base. The object is 0x28 bytes and its vtable is at
 * `0x00814530`. The payload comes from the copy constructor at `0x003f38d0`, which Clone()
 * delegates to, so the offsets and widths are recovered but the purpose of each field is not. The
 * four words Packet owns are declared there rather than here.
 *
 * The class overrides Message::Print() at `0x003f2648`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class CatchProgressPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0a70
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nCatchProgressPacketType.
     * @ghidraAddress 0x003f0ae8
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `CatchProgressPacket`.
     * @ghidraAddress 0x003f0af8
     */
    virtual const char *Name();

private:
    long long mUnknown14; // +0x14
    // Save() hands the address of this member to CmdID::Save(), which the inline re-emission at
    // 0x004acf28 establishes, so the member is a CmdID rather than a plain word.
    CmdID mUnknown1c; // +0x1c
    int mUnknown20;   // +0x20
    float mUnknown24; // +0x24
};

/**
 * Identity that CatchProgressPacket::Type() reports.
 *
 * This word belongs to CatchProgressPacket because CatchProgressPacket::Type() at `0x003f0ae8`
 * returns it.
 *
 * @ghidraAddress 0x006d73f4
 */
extern int g_nCatchProgressPacketType;
