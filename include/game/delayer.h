#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"

class Message;

/**
 * Sink that forwards the messages it receives to its own sinks.
 *
 * `7Delayer` in the RTTI descriptor, with MsgSink at offset 0 and MsgSource at `+0x04`. Its
 * primary table at `0x00817720` runs four entries: the type function, the destructor at
 * `0x0040cee8`, the inherited MsgSink::Handle(), and the HandleMessage() override below. The
 * MsgSource table at `0x008176f8` adjusts `this` by `-4` for the first two entries and retains
 * MsgSource::AddSink() and MsgSource::RemoveSink(). GrooveWorld's setup routine at `0x0018cce8`
 * creates the one instance with a 0x18-byte allocation, which leaves no room for a member of the
 * class's own.
 *
 * The class is declared for GrooveWorld, which hands it every CripplePacket. The copy
 * constructor at `0x0040d060` is the implicit one and is not written. It stores
 * the MsgSink table, copies the MsgSource base, and then installs the two tables above.
 */
class Delayer : public MsgSink, public MsgSource {
public:
    /**
     * @ghidraAddress 0x0040cee8
     */
    virtual ~Delayer();

    /**
     * Act on a message. Primary table slot 3.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x0040d0f0
     */
    virtual void HandleMessage(Message *pMsg);
};
