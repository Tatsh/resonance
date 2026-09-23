#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * `27ToAllOtherGameSystemsPacket` in the RTTI descriptor at `0x00901d90`, with Packet as its one
 * base. It has no vtable of its own. Its RTTI accessor is shared with a concrete packet, which is
 * why a per-name scan credits that packet's vtable to this name as well, and why a size read
 * there is really the derived class's.
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
