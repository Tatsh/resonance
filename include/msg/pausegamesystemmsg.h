#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18PauseGameSystemMsg` in the RTTI descriptor at `0x00901da0`, with Message as its one base. The
 * object is 0x4 bytes and its vtable is at `0x007dc4c0`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 */
class PauseGameSystemMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 434.
     *
     * @return The message.
     * @ghidraAddress 0x003d7bc0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00193ce0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPauseGameSystemMsgType.
     * @ghidraAddress 0x00193d18
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PauseGameSystemMsg`.
     * @ghidraAddress 0x00193d28
     */
    virtual const char *Name();
};

/**
 * Identity that PauseGameSystemMsg::Type() reports.
 *
 * This word belongs to PauseGameSystemMsg because PauseGameSystemMsg::Type() at `0x00193d18`
 * returns it, and the registration at `0x003d9818` passes the same value, 434, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d03b4
 */
extern int g_nPauseGameSystemMsgType;
