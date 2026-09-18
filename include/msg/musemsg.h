#pragma once

#include "msg/message.h"

/**
 * Abstract base shared by a family of messages.
 *
 * `7MuseMsg` in the RTTI descriptor at `0x008ef330`, with Message as its one base. Its vtable is
 * at `0x007dd628`, which every one of the 3 derived Clone() routines installs before the derived
 * table. Slots 2, 3, and 4 there address the pure-virtual handler and slot 5 addresses
 * Message::Print(), so the class implements none of the virtuals it inherits and is never
 * instantiated. It is declared because its derived classes need it.
 *
 * Slot 0 of that table is `0x0019a988` and slot 1 the destructor at `0x0019a958`, neither of
 * which matches the accessor `rtti.json` lists for this name. That is the same
 * unreliable-accessor defect recorded for MsgSink and MsgQueue, so the table rather than the
 * harvest is the authority here.
 *
 * The payload is inferred from the derived classes rather than from any routine of its own,
 * because Clone() in a derived class copies the whole object. Only these two fields are copied
 * identically by all three derived classes. NoteMsg and StdMidiMsg also copy `+0x09` and `+0x0a`
 * while SustainNoteMsg does not, so those two bytes may belong here with one Clone() omitting
 * them. They are declared in the derived classes that copy them rather than attributed here.
 */
class MuseMsg : public Message {
protected:
    int mUnknown04;           // +0x04
    unsigned char mUnknown08; // +0x08
};
