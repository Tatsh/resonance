#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901da0`. It has Message as its one base. The object is 0x4 bytes
 * and its vtable is at `0x007dc4c0`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * This class adds no field of its own, and the size is exactly the size of the base, which is what
 * fixes the size of the base.
 *
 * The destructor at `0x00193c30` is compiler-generated and has no declaration here.
 */
class PauseGameSystemMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 434.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7bc0
     * @ghidraAddress PAL: 0x0040fad8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x00193ce0
     * @ghidraAddress PAL: 0x00199908
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPauseGameSystemMsgType.
     * @ghidraAddress NTSC-U/C: 0x00193d18
     * @ghidraAddress PAL: 0x00199940
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PauseGameSystemMsg`.
     * @ghidraAddress NTSC-U/C: 0x00193d28
     * @ghidraAddress PAL: 0x00199950
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
 * @ghidraAddress NTSC-U/C: 0x006d03b4
 * @ghidraAddress PAL: 0x00713b4c
 */
extern int g_nPauseGameSystemMsgType;
