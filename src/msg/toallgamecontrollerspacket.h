#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * `26ToAllGameControllersPacket` in the RTTI descriptor at `0x008f0750`, with Packet as its one
 * base. It has no vtable of its own. Its RTTI accessor is shared with a concrete packet, which is
 * why a per-name scan credits that packet's vtable to this name as well, and why a size read
 * there is really the derived class's.
 *
 * The class adds no payload. Its three derived classes (SCStartPlayingPacket, SCLoadLevelPacket,
 * and SCGameOverPacket) start their own payload at `+0x14`.
 */
class ToAllGameControllersPacket : public Packet {
public:
    /**
     * Route the packet with the pair 3 and 2.
     *
     * Inline. The New() factories of all three derived classes store that pair.
     */
    ToAllGameControllersPacket() : Packet(3, 2) {
    }
};
