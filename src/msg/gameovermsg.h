#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef630`. It has Message as its one base. The object is 0x4 bytes
 * and its vtable is at `0x007dc508`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 *
 * The destructor at `0x00193b28` is compiler-generated and has no declaration here.
 */
class GameOverMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 422.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7908
     * @ghidraAddress PAL: 0x0040f808
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x00193bd8
     * @ghidraAddress PAL: 0x00199800
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGameOverMsgType.
     * @ghidraAddress NTSC-U/C: 0x00193c10
     * @ghidraAddress PAL: 0x00199838
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GameOverMsg`.
     * @ghidraAddress NTSC-U/C: 0x00193c20
     * @ghidraAddress PAL: 0x00199848
     */
    virtual const char *GetName() const;
};

/**
 * Identity that GameOverMsg::Type() reports.
 *
 * This word belongs to GameOverMsg because GameOverMsg::Type() at `0x00193c10` returns it, and the
 * registration at `0x003d9818` passes the same value, 422, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0354
 * @ghidraAddress PAL: 0x00713aec
 */
extern int g_nGameOverMsgType;
