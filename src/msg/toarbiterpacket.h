#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * Its RTTI descriptor is at `0x00902a00`. It has Packet as its one base. The class emits no vtable,
 * and its RTTI accessor is shared with a concrete packet.
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
