#pragma once

#include "msg/toallothergamesystemspacket.h"
#include "sch/cmdid.h"

/**
 * Network packet the game sends between game systems.
 *
 * `17TrackSelectPacket` in the RTTI descriptor at `0x008ef620`, with ToAllOtherGameSystemsPacket
 * as its one base. The object is 0x28 bytes and its vtable is at `0x00814578`. The payload comes
 * from the copy constructor at `0x003f3868`, which Clone() delegates to, so the offsets and
 * widths are recovered but the purpose of each field is not. The four words Packet owns are
 * declared there rather than here.
 *
 * The class overrides Message::Print() at `0x003f2568`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class TrackSelectPacket : public ToAllOtherGameSystemsPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003f0848
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nTrackSelectPacketType.
     * @ghidraAddress 0x003f08c0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `TrackSelectPacket`.
     * @ghidraAddress 0x003f08d0
     */
    virtual const char *Name();

private:
    // Save() hands the address of this member to CmdID::Save(), which the inline re-emission at
    // 0x004acf28 establishes, so the member is a CmdID rather than a plain word.
    CmdID mUnknown14;     // +0x14
    long long mUnknown18; // +0x18
    int mUnknown20;       // +0x20
    int mUnknown24;       // +0x24
};

/**
 * Identity that TrackSelectPacket::Type() reports.
 *
 * This word belongs to TrackSelectPacket because TrackSelectPacket::Type() at `0x003f08c0`
 * returns it.
 *
 * @ghidraAddress 0x006d73ec
 */
extern int g_nTrackSelectPacketType;
