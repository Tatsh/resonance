#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * Its RTTI descriptor is at `0x00901d90`. It has Packet as its one base. The class emits no vtable,
 * and its RTTI accessor is shared with a concrete packet.
 *
 * The class adds no payload. The shared prefix of its 7 derived classes is the four words Packet
 * already provides.
 */
class ToAllOtherGameSystemsPacket : public Packet {
public:
    /**
     * Route the packet with the pair 2 and 3.
     *
     * Inline. The New() factories of all seven derived classes store that pair.
     */
    ToAllOtherGameSystemsPacket() : Packet(2, 3) {
    }
};
