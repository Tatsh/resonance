#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * Its RTTI descriptor is at `0x008f0790`. It has Packet as its one base. The class emits no vtable,
 * and its RTTI accessor is shared with a concrete packet.
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
