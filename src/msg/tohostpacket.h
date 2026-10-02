#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * `12ToHostPacket` in the RTTI descriptor at `0x008f0740`, with Packet as its one base. It has no
 * vtable of its own. Its RTTI accessor is shared with a concrete packet, which is why a per-name
 * scan credits that packet's vtable to this name as well, and why a size read there is really the
 * derived class's.
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
