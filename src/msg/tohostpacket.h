#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * Its RTTI descriptor is at `0x008f0740`. It has Packet as its one base. The class emits no vtable,
 * and its RTTI accessor is shared with a concrete packet.
 *
 * The class adds no payload. Its four derived classes (PSJoinRequestPacket, CSClientStatusPacket,
 * CSInitiatePlayPacket, and BSLoadLevelPacket) all start their own payload at `+0x14`.
 */
class ToHostPacket : public Packet {
public:
    /**
     * Route the packet with the pair 0 and 0.
     *
     * Inline. The New() factories of all four derived classes store that pair.
     */
    ToHostPacket() : Packet(0, 0) {
    }
};
