#pragma once

#include "msg/message.h"

/**
 * Abstract base shared by a family of messages.
 *
 * `6CmdMsg` in the RTTI descriptor at `0x008ef860`, with Message as its one base. Its vtable is
 * at `0x007e46b8`, which every one of the 5 derived Clone() routines installs before the derived
 * table. That table has eight entries and a zero terminator at index 8. Slots 2, 3, and 4 address
 * the pure-virtual handler, and slots 5, 6, and 7 retain Message::Print(), Message::Save(), and
 * Message::Load(), so the class implements none of the virtuals it inherits and is never
 * instantiated. It is declared because its derived classes need it.
 *
 * The destructor at slot 1 is byte-identical to Message's, because it stores its own table pointer,
 * the inlined base destructor overwrites it, and the compiler drops the dead first store. It is
 * therefore the implicitly declared destructor, and this class owes no definition.
 *
 * Slot 0 of that table is `0x001cacf8` and slot 1 the destructor at `0x001cacc8`, neither of
 * which matches the accessor `rtti.json` lists for this name. That is the same
 * unreliable-accessor defect recorded for MsgSink and MsgQueue, so the table rather than the
 * harvest is the authority here.
 *
 * The one member is the word at `+0x04`. Every New() of the five derived classes and every stack
 * construction in the image zeroes it, and CripplePowerup reads it back after MsgSource::Send()
 * at `0x001c9878` to learn whether a receiver acted, so it is a result the receiver fills in.
 *
 * The words at `+0x08` and `+0x0c` belong to the derived classes, because their types differ
 * between them. CrippleMsg's Print() dereferences `+0x08` as a player and writes `+0x0c` as a
 * track number, while PhraseNeutralizer builds a PlayersTrackNeutralizedMsg at `0x001c0b2c` with
 * a player at `+0x0c`. No single declaration here could type both.
 */
class CmdMsg : public Message {
public:
    /**
     * Clear the result word.
     *
     * Inline. Every construction of a derived class stores zero at `+0x04`.
     */
    CmdMsg() : mUnknown04(0) {
    }

    /**
     * Result a receiver writes back, zero until one does.
     *
     * Public because CripplePowerup reads it at `0x001c9878` from outside the hierarchy after the
     * message is sent, and the image exposes no accessor.
     *
     * +0x04
     */
    int mUnknown04;
};
