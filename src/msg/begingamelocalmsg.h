#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `17BeginGameLocalMsg` in the RTTI descriptor at `0x00901b50`, with Message as its one base. The
 * object is 0x4 bytes and its vtable is at `0x007cd568`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 *
 * The destructor at `0x0010b998` is compiler-generated and has no declaration here. The routine
 * at `0x0010b9d0` is a further emission of the type-information accessor.
 */
class BeginGameLocalMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 432.
     *
     * @return The message.
     * @ghidraAddress 0x003d7b50
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0010ba48
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nBeginGameLocalMsgType.
     * @ghidraAddress 0x0010ba80
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `BeginGameLocalMsg`.
     * @ghidraAddress 0x0010ba90
     */
    virtual const char *Name();
};

/**
 * Identity that BeginGameLocalMsg::Type() reports.
 *
 * This word belongs to BeginGameLocalMsg because BeginGameLocalMsg::Type() at `0x0010ba80` returns
 * it, and the registration at `0x003d9818` passes the same value, 432, as the identity of this
 * class's factory.
 *
 * @ghidraAddress 0x006d03a4
 */
extern int g_nBeginGameLocalMsgType;
