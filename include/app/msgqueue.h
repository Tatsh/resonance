#pragma once

#include <vector>

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Relay that stores a copy of every message it accepts and forwards to its own sinks.
 *
 * `8MsgQueue` in the RTTI descriptor at `0x00902220`, deriving from MsgSource at offset 0 and from
 * MsgSink at `+0x14`, which the base records in the descriptor give directly. The object is 0x3c
 * bytes, fixed by GameManagerImpl embedding one at `+0xc0` and writing its own next member at
 * `+0xfc`. Two vtables belong to the class. The table at `0x00829b18` is addressed by the
 * MsgSource subobject and inherits both of that base's virtuals unchanged; the table at
 * `0x00829af0` is addressed by the MsgSink subobject, adjusts `this` by `-0x14` on every entry it
 * overrides, and inherits MsgSink::Handle() with no adjustment at all.
 *
 * Deriving from both mix-ins is the whole design. A message arrives through the MsgSink side,
 * which stores a copy, and arrives at its readers through the inherited MsgSource sink list. The
 * class therefore has no drain member of its own, and none exists in its translation unit.
 * GameManagerImpl demonstrates the arrangement: its constructor builds the embedded queue and then
 * registers itself as a sink of it at `0x00105fec`.
 *
 * The queue owns its stored messages. The destructor deletes every element of both vectors through
 * Message's virtual destructor, which is what establishes the ownership and in turn establishes
 * that Message::Clone() returns a heap copy.
 *
 * One thing is unresolved. The constructor points mTarget at mFirst, and HandleMessage() appends
 * through mTarget, so the class can switch which vector accepts a message. Nothing recovered ever
 * writes mTarget again, so the second vector is never used and the reason for the pair stays
 * open. The two trailing words are likewise unrecovered.
 */
class MsgQueue : public MsgSource, public MsgSink {
public:
    /**
     * Start with both vectors empty and mFirst accepting.
     *
     * @ghidraAddress 0x0054a738
     */
    MsgQueue();

    /**
     * Delete every stored message and release both vectors.
     *
     * @ghidraAddress 0x0054a7a0
     */
    virtual ~MsgQueue();

    /**
     * Store a copy of a message without reading its identity first.
     *
     * The body is HandleMessage() with the Message::Type() call omitted.
     *
     * @param pMsg The message to copy and store.
     * @ghidraAddress 0x0054b220
     */
    void Store(Message *pMsg);

protected:
    /**
     * Store a copy of a message.
     *
     * @param pMsg The message to copy and store.
     * @ghidraAddress 0x0054b290
     */
    virtual void HandleMessage(Message *pMsg);

private:
    std::vector<Message *> mFirst;   // +0x18
    std::vector<Message *> mSecond;  // +0x24
    std::vector<Message *> *mTarget; // +0x30
    int mUnknown34;                  // +0x34 not written by the constructor
    int mUnknown38;                  // +0x38 cleared by the constructor
};
