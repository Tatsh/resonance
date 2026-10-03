#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef7e0`. It has Message as its one base. The object is 0x4 bytes
 * and its vtable is at `0x007dc550`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 *
 * The destructor at `0x00193a20` is compiler-generated and has no declaration here.
 */
class GameBeginMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 420.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7888
     * @ghidraAddress PAL: 0x0040f788
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x00193ad0
     * @ghidraAddress PAL: 0x001996f8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameBeginMsgType.
     * @ghidraAddress NTSC-U/C: 0x00193b08
     * @ghidraAddress PAL: 0x00199730
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameBeginMsg`.
     * @ghidraAddress NTSC-U/C: 0x00193b18
     * @ghidraAddress PAL: 0x00199740
     */
    virtual const char *GetName() const;
};

/**
 * Identity that GameBeginMsg::Type() reports.
 *
 * This word belongs to GameBeginMsg because GameBeginMsg::Type() at `0x00193b08` returns it, and
 * the registration at `0x003d9818` passes the same value, 420, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0344
 * @ghidraAddress PAL: 0x00713adc
 */
extern int g_nGameBeginMsgType;
