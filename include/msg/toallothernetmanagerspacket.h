#pragma once

#include "msg/packet.h"

/**
 * Routing category for a packet, which selects who receives it.
 *
 * `27ToAllOtherNetManagersPacket` in the RTTI descriptor at `0x008f09a0`, with Packet as its one
 * base. It has no vtable of its own, and its RTTI accessor is shared with a concrete packet, so a
 * per-name scan credits that packet's vtable to this name as well.
 *
 * Nothing is declared here. Every one of its 3 derived classes starts its own payload at `+0x14`,
 * so this class adds nothing beyond the four words Packet owns.
 */
class ToAllOtherNetManagersPacket : public Packet {};
