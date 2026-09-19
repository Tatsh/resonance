#pragma once

#include <vector>

#include "game/freqappearance.h"
#include "game/gameparams.h"
#include "msg/tosinglenetmanagerpacket.h"
#include "os/hxstr.h"

/**
 * Network packet the game sends between game systems.
 *
 * `18SPJoinAcceptPacket` in the RTTI descriptor at `0x008ef110`, with ToSingleNetManagerPacket as
 * its one base. The object is 0x7c bytes and its vtable is at `0x00814890`. The payload comes
 * from the copy constructor at `0x003f2e48`, which Clone() delegates to, and it accounts for the
 * allocation exactly. The four words Packet owns are declared there rather than here.
 *
 * The vector at `+0x70` is deep-copied, so the class owns its elements. The element stride is 0x8
 * bytes, taken from the divide the copy uses to count them, but the element type is not
 * recovered, so it is declared as a record of that size.
 *
 * The class overrides Message::Print() at `0x003f1f58`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SPJoinAcceptPacket : public ToSingleNetManagerPacket {
public:
    /**
     * Produce a heap copy of this packet.
     *
     * @return The copy.
     * @ghidraAddress 0x003ef078
     */
    virtual Message *Clone();

    /**
     * Report this packet's registered identity.
     *
     * @return g_nSPJoinAcceptPacketType.
     * @ghidraAddress 0x003ef0f0
     */
    virtual int Type();

    /**
     * Report this packet's class name.
     *
     * @return The literal `SPJoinAcceptPacket`.
     * @ghidraAddress 0x003ef100
     */
    virtual const char *Name();

private:
    // Element of the vector at +0x70. Only its size is recovered.
    struct Entry70 {
        int mUnknown00;
        int mUnknown04;
    };

    int mUnknown14;                  // +0x14
    int mUnknown18;                  // +0x18
    GameParams mUnknown1c;           // +0x1c
    HxStr mUnknown54;                // +0x54
    FreqAppearance mUnknown5c;       // +0x5c
    std::vector<Entry70> mUnknown70; // +0x70
};

/**
 * Identity that SPJoinAcceptPacket::Type() reports.
 *
 * This word belongs to SPJoinAcceptPacket because SPJoinAcceptPacket::Type() at `0x003ef0f0`
 * returns it.
 *
 * @ghidraAddress 0x006d7374
 */
extern int g_nSPJoinAcceptPacketType;
