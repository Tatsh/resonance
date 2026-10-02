#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * `24ToSingleNetManagerPacket` in the RTTI descriptor at `0x008efca0`, with Packet as its one
 * base. It has no vtable of its own. Its RTTI accessor is shared with a concrete packet, which is
 * why a per-name scan credits that packet's vtable to this name as well, and why a size read
 * there is really the derived class's.
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
