#pragma once

#include "app/message.h"

/**
 * Abstract base shared by a family of messages.
 *
 * `6CmdMsg` in the RTTI descriptor at `0x008ef860`, with Message as its one base. Its vtable is
 * at `0x007e46b8`, which every one of the 5 derived Clone() routines installs before the derived
 * table. Slots 2, 3, and 4 there address the pure-virtual handler and slot 5 addresses
 * Message::Print(), so the class implements none of the virtuals it inherits and is never
 * instantiated. It is declared because its derived classes need it.
 *
 * Slot 0 of that table is `0x001cacf8` and slot 1 the destructor at `0x001cacc8`, neither of
 * which matches the accessor `rtti.json` lists for this name. That is the same
 * unreliable-accessor defect recorded for MsgSink and MsgQueue, so the table rather than the
 * harvest is the authority here.
 *
 * The payload is inferred from the derived classes rather than from any routine of its own,
 * because Clone() in a derived class copies the whole object. Every one of the five derived
 * classes copies these three fields, and PlayersTrackNeutralizedMsg is exactly 0x10 bytes and
 * adds nothing, which fixes the size of this base.
 */
class CmdMsg : public Message {
protected:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};
