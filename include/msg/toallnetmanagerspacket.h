#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * `22ToAllNetManagersPacket` in the RTTI descriptor at `0x008f0790`, with Packet as its one base.
 * It has no vtable of its own, and its RTTI accessor is shared with a concrete packet, so a
 * per-name scan credits that packet's vtable to this name as well.
 *
 * The class adds no payload. Its one derived class, SCPlayerJoinedPacket, starts its own payload
 * at `+0x14`.
 */
class ToAllNetManagersPacket : public Packet {
public:
    /**
     * Route the packet with the pair 3 and 0.
     *
     * Inline. SCPlayerJoinedPacket::New() stores that pair.
     */
    ToAllNetManagersPacket() : Packet(3, 0) {
    }
};
