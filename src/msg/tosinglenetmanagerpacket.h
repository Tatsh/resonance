#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * Its RTTI descriptor is at `0x008efca0`. It has Packet as its one base. The class emits no vtable,
 * and its RTTI accessor is shared with a concrete packet.
 *
 * The class adds no payload. Its two derived classes, SPJoinAcceptPacket and SPJoinDenyPacket,
 * both start their own payload at `+0x14`.
 */
class ToSingleNetManagerPacket : public Packet {
public:
    /**
     * Route the packet with the pair 1 and 0.
     *
     * Inline. The New() factories of both derived classes store that pair.
     */
    ToSingleNetManagerPacket() : Packet(1, 0) {
    }
};
