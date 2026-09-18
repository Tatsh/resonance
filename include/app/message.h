#pragma once

/**
 * Base of every event the game passes between a MsgSource and a MsgSink.
 *
 * `7Message` in the RTTI descriptor at `0x0086f638`, with no base and no data members, so the
 * compiler-generated vptr lands at offset 0 and every derived message adds its payload after it.
 * Around eighty classes derive from it, among them ScriptMsg, CmdMsg, TextMsg, and Packet.
 *
 * The class declares five virtuals. The destructor takes slot 1, Clone() and Type() take slots 2
 * and 3, slot 4 is pure and unrecovered, and slot 5 has a default implementation at `0x001051f0`
 * that is also unrecovered. The two trailing virtuals are omitted here rather than guessed at,
 * which disturbs neither of the recovered slots. The count comes from the Packet table at
 * `0x00814920`, which inherits all three pure slots.
 */
class Message {
public:
    virtual ~Message();

    /**
     * Produce a heap copy of this message.
     *
     * MsgQueue stores the result and its destructor deletes each stored message through this
     * virtual destructor, which is what establishes that the result is an owned heap copy rather
     * than the original message.
     *
     * @return The copy.
     */
    virtual Message *Clone() = 0;

    /**
     * Report this message's identity.
     *
     * A sink compares the result against the identity it listens for, which is what
     * ScriptSink::HandleMessage() does against the registered value at `0x006d024c`. MsgQueue
     * also invokes this and discards the result, so the member has an effect beyond reporting the
     * identity that is not recovered yet.
     *
     * @return The identity.
     */
    virtual int Type() = 0;
};
