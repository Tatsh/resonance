#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * `15ToArbiterPacket` in the RTTI descriptor at `0x00902a00`, with Packet as its one base. It has
 * no vtable of its own, and its RTTI accessor is shared with a concrete packet, so a per-name
 * scan credits that packet's vtable to this name as well.
 *
 * The class adds no payload. Its one derived class, TestArbiterPacket, starts its own payload at
 * `+0x14`.
 */
class ToArbiterPacket : public Packet {
public:
    /**
     * Route the packet with the pair 0 and 1.
     *
     * Inline. TestArbiterPacket::New() stores that pair.
     */
    ToArbiterPacket() : Packet(0, 1) {
    }
};
