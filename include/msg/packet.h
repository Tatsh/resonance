#pragma once

#include "msg/message.h"

/**
 * Abstract base of every network packet.
 *
 * `6Packet` in the RTTI descriptor at `0x009021a0`, with Message as its one base. Slots 2, 3, and
 * 4 of its table at `0x00814920` address the pure-virtual handler, so it implements none of the
 * virtuals it inherits and is never instantiated.
 *
 * The four words below are recovered from outside rather than from any routine of its own. Three
 * unrelated routing families give the identical shared prefix, and two packets,
 * CSInitiatePlayPacket and SCStartPlayingPacket, are exactly 0x14 bytes and add nothing to it,
 * which measures this class exactly. A shared prefix common to three separate families belongs to
 * the grandparent rather than to any one of them.
 */
class Packet : public Message {
protected:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
};
