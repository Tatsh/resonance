#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12GameBeginMsg` in the RTTI descriptor at `0x008ef7e0`, with Message as its one base. The
 * object is 0x4 bytes and its vtable is at `0x007dc550`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 */
class GameBeginMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 420.
     *
     * @return The message.
     * @ghidraAddress 0x003d7888
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00193ad0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameBeginMsgType.
     * @ghidraAddress 0x00193b08
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameBeginMsg`.
     * @ghidraAddress 0x00193b18
     */
    virtual const char *Name();
};

/**
 * Identity that GameBeginMsg::Type() reports.
 *
 * This word belongs to GameBeginMsg because GameBeginMsg::Type() at `0x00193b08` returns it, and
 * the registration at `0x003d9818` passes the same value, 420, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0344
 */
extern int g_nGameBeginMsgType;
