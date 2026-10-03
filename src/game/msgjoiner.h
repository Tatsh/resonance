#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"

class Message;

/**
 * Sink that sends every message it receives on to its own sinks.
 *
 * It has MsgSource at offset 0 and MsgSink at `+0x14`. The primary table at `0x007dcc78` retains
 * MsgSource::AddSink() and MsgSource::RemoveSink(), and the MsgSink table at `0x007dcc50` adjusts
 * `this` by `-20`, retains MsgSink::Dispatch(), and fills slot 3 with DispatchPriv() below.
 * GrooveWorld's setup routine at `0x0018cce8` creates the one instance with a 0x18-byte allocation.
 * The allocation has no room for a member beyond the two bases.
 *
 * The constructor at `0x001940e0` and the destructor at `0x00195a30` are both implicitly declared.
 * The first runs the MsgSource constructor and installs the two tables, and the second restores
 * the base tables, frees MsgSource's vector, and releases the object under MsgSink's tag, which is
 * what the compiler generates for a class with no member of its own.
 */
class MsgJoiner : public MsgSource, public MsgSink {
public:
    /**
     * Send the message to every registered sink.
     *
     * Slot 3 of the MsgSink table.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x00195b70
     * @ghidraAddress PAL: 0x0019b808
     */
    virtual void DispatchPriv(Message *pMsg);
};
