#pragma once

#include <vector>

#include "app/message.h"
#include "app/msgsink.h"
#include "app/msgsource.h"

/**
 * Sink that stores the messages it accepts for later delivery.
 *
 * `8MsgQueue` in the RTTI descriptor at `0x00902220`, deriving from MsgSource at offset 0 and from
 * MsgSink at `+0x14`, which the base records in the descriptor give directly. Two vtables belong
 * to the class. The table at `0x00829b18` is addressed by the MsgSource subobject and inherits
 * both of that base's virtuals unchanged; the table at `0x00829af0` is addressed by the MsgSink
 * subobject, adjusts `this` by `-0x14` on every entry it overrides, and inherits MsgSink::Handle()
 * with no adjustment at all.
 *
 * The two vectors below store owned messages. The destructor deletes every element of each through
 * Message's virtual destructor, which is what establishes the ownership and in turn establishes
 * that Message::Clone() returns a heap copy.
 *
 * Recovery is incomplete in one specific way. `+0x30` stores the address of a vector rather than a
 * vector, and HandleMessage() appends through it, so the class chooses at run time which of its two
 * vectors accepts a message. Nothing recovered so far sets `+0x30` or drains either vector, and
 * neither the constructor nor a drain member is identified, so the reason for the pair is open.
 */
class MsgQueue : public MsgSource, public MsgSink {
public:
    /**
     * Delete every stored message and release both vectors.
     *
     * @ghidraAddress 0x0054a7a0
     */
    virtual ~MsgQueue();

    /**
     * Store a copy of a message without reporting its identity first.
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
};
