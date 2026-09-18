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
 * Nothing is declared here. The class has one derived class, so a shared prefix would be that one
 * class's whole payload and would attribute all of it to this base. The payload stays in the
 * derived class until a second one exists to measure against.
 */
class ToSingleNetManagerPacket : public Packet {};
