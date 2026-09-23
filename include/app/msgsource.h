#pragma once

#include <vector>

class Message;
class MsgSink;

/**
 * Sender of engine messages, which stores the list of sinks that receive them.
 *
 * `9MsgSource` in the RTTI descriptor at `0x0086f6b0`, with no base. One data word sits ahead of
 * the vector, so the compiler places the vptr after both at `+0x10` and the subobject is 0x14
 * bytes. Around thirty classes derive from it, among them Player, InputMap, Renderer, and
 * MsgQueue. MsgQueue derives from both this class and MsgSink, and its MsgSink subobject therefore
 * sits at `+0x14`.
 *
 * The vector is three pointers, measured from the destructor and from both accessors: `+0x04`
 * start, `+0x08` finish, `+0x0c` end of storage. Every operation on it is inlined apart from the
 * reallocating half of push_back() at `0x00549e08`. The vector is private. Only members of this
 * class address it.
 *
 * The class declares no constructor, and the two the compiler generates are emitted once each, in
 * the Gamer unit. The default constructor at `0x00115cc0` empties the vector and writes the vptr,
 * and AutoRiffer, AxeFX, and the other derived constructors call it. The copy constructor at
 * `0x00115860` copies the word at `+0x00` and the vector.
 */
class MsgSource {
public:
    /**
     * @ghidraAddress 0x0054a168
     */
    virtual ~MsgSource();

    /**
     * Register a sink, ignoring a sink that is already registered.
     *
     * @param pSink The sink to register.
     * @ghidraAddress 0x0054a270
     */
    virtual void AddSink(MsgSink *pSink);

    /**
     * Unregister the first occurrence of a sink.
     *
     * A sink that is not registered is ignored.
     *
     * @param pSink The sink to unregister.
     * @ghidraAddress 0x0054a2f0
     */
    virtual void RemoveSink(MsgSink *pSink);

    /**
     * Deliver a message to every registered sink, in registration order.
     *
     * The member is not virtual. This class's own table is at `0x008299f0` and runs to its
     * terminator after RemoveSink(), and every one of the hundreds of call sites is a direct call.
     * The table at `0x00829b18`, recorded here previously, belongs to the MsgSource subobject of
     * MsgQueue rather than to this class. The conclusion holds for either table, because neither
     * has a further entry.
     *
     * The loop reloads the vector's finish pointer on each iteration. A sink that registers a
     * further sink while receiving the message is therefore still included. Delivery to one sink
     * goes through MsgSink::Handle(), table slot 2.
     *
     * @param pMsg The message to deliver.
     * @ghidraAddress 0x0054a370
     */
    void Send(Message *pMsg);

    /**
     * Unregister every sink.
     *
     * GrooveWorld::DestroyGraphs() at `0x0018da60` calls it. An identical copy sits at
     * `0x001fedb0`.
     *
     * @ghidraAddress 0x0054a218
     */
    void ClearSinks();

    /**
     * A word no member of this class reads. +0x00
     *
     * Public because GrooveWorld::BuildGraphs() stores 1 into it on the new InputMap at
     * `0x0018ce18`, and the image has no accessor.
     */
    int mUnknown00;

private:
    std::vector<MsgSink *> mSinks; // +0x04
};
