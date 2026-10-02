#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * Its RTTI descriptor is at `0x008f0750`. It has Packet as its one base. The class emits no vtable,
 * and its RTTI accessor is shared with a concrete packet.
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
