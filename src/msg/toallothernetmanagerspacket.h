#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * Its RTTI descriptor is at `0x008f09a0`. It has Packet as its one base. The class emits no vtable,
 * and its RTTI accessor is shared with a concrete packet.
 *
 * The class adds no payload. Every one of its 3 derived classes starts its own payload at
 * `+0x14`.
 */
class ToAllOtherNetManagersPacket : public Packet {
public:
    /**
     * Route the packet with the pair 2 and 0.
     *
     * Inline. The New() factories of all three derived classes store that pair.
     */
    ToAllOtherNetManagersPacket() : Packet(2, 0) {
    }
};
