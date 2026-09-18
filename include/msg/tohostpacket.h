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
 * Nothing is declared here. The shared prefix of its 2 derived classes is the four words Packet
 * already owns, so this class adds no payload of its own.
 */
class ToHostPacket : public Packet {};
