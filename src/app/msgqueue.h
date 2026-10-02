#pragma once

#include <vector>

#include "app/msgsink.h"
#include "app/msgsource.h"

class Message;

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
 * which stores a copy, and arrives at its readers through the inherited MsgSource sink list.
 * GameManagerImpl demonstrates the arrangement: its constructor builds the embedded queue and then
 * registers itself as a sink of it at `0x00105fec`.
 *
 * The queue owns its stored messages. The destructor deletes every element of both vectors through
 * Message's virtual destructor, which is what establishes the ownership and in turn establishes
 * that Message::Clone() returns a heap copy.
 *
 * Poll() at `0x0054aa58` is the drain, and an earlier reading of this class stated that no drain
 * member existed. It also resolves what the pair of vectors is for. Poll() points mTarget at the
 * vector it is not iterating, so a message that a sink stores while receiving an earlier one lands
 * in the other vector and the iteration is undisturbed.
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

    /**
     * Deliver and discard every stored message.
     *
     * The routine marks itself as running, swaps the accepting vector, then sends each stored
     * message on to the queue's own sinks and deletes it. The drained vector is emptied
     * afterwards. Rnd::World::DrawFrame() and slot 6 of the metagame renderer are the callers.
     * Both run once per frame.
     *
     * The title is inferred from the behaviour. The member is not virtual and both call sites are
     * direct calls. No recovered metadata records a title for it.
     *
     * @ghidraAddress 0x0054aa58
     */
    void Poll();

protected:
    /**
     * Store a copy of a message.
     *
     * @param pMsg The message to copy and store.
     * @ghidraAddress 0x0054b290
     */
    virtual void HandleMessage(Message *pMsg);

private:
    std::vector<Message *> mFirst;
    std::vector<Message *> mSecond;
    // The vector that accepts a stored message. The constructor points it at mFirst and Poll()
    // swaps it.
    std::vector<Message *> *mTarget;
    // The vector Poll() is iterating. Poll() is the only writer, and it reloads the member on
    // every iteration rather than retaining it in a register.
    std::vector<Message *> *mDraining;
    // Non-zero while Poll() runs. Nothing recovered reads it.
    int mInPoll;
};
