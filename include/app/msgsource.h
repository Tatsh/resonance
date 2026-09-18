#pragma once

#include <vector>

#include "app/msgsink.h"

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
 * reallocating half of push_back() at `0x00549e08`. Both members are private, because the only
 * code that reads either is a member of this class.
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

private:
    int mUnknown00;                // +0x00
    std::vector<MsgSink *> mSinks; // +0x04
};
