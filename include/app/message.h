#pragma once

#include <iostream.h>

/**
 * Base of every event the game passes between a MsgSource and a MsgSink.
 *
 * `7Message` in the RTTI descriptor at `0x0086f638`, with no base and no data members, so the
 * compiler-generated vptr lands at offset 0 and every derived message adds its payload after it.
 * The RTTI lists 124 derived classes, among them ScriptMsg, GemMsg, LeaveGameMsg, and Packet.
 *
 * All four virtuals are recovered, and the table order gives the declaration order. Every one of
 * them was read off the concrete implementations rather than off the declaration: a scan of the
 * derived tables found 80 classes that implement Clone(), Type(), and Name(), while Packet and
 * eight others inherit all three as pure and are therefore abstract themselves.
 *
 * The three pure virtuals follow one pattern per class. LeaveGameMsg at `0x00812078` and GemMsg at
 * `0x00812588` are the two worked examples cited below.
 */
class Message {
public:
    virtual ~Message();

    /**
     * Produce a heap copy of this message.
     *
     * An implementation allocates the derived size against the tag `MSG`, installs its own vtable,
     * and copies its payload word by word. GemMsg's copies 0x18 bytes and LeaveGameMsg's copies
     * only the vptr, because that class has no payload. MsgQueue stores the result and its
     * destructor deletes every stored message, so the queue owns the copies.
     *
     * @return The copy.
     */
    virtual Message *Clone() = 0;

    /**
     * Report this message's registered identity.
     *
     * An implementation returns one word from the per-class identity table that spans `0x006d01ac`
     * through `0x006d035c`. A sink compares the result against the identity it listens for, which
     * is what ScriptSink::HandleMessage() does against `0x006d024c`. The read has no side effect,
     * so the call MsgQueue::HandleMessage() makes and discards has none either.
     *
     * @return The identity.
     */
    virtual int Type() = 0;

    /**
     * Report this message's class name.
     *
     * An implementation returns its own name as a string literal. LeaveGameMsg returns
     * `LeaveGameMsg` from `0x008116a8` and GemMsg returns `GemMsg` from `0x00811570`.
     *
     * @return The name.
     */
    virtual const char *Name() = 0;

    /**
     * Write this message's payload to a stream.
     *
     * The default writes nothing. GemMsg's override at `0x003d8830` streams its fields, so the
     * member exists for diagnostics.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x001051f0
     */
    virtual void Print(ostream &stream);
};
