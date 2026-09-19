#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `15PlaybackModeMsg` in the RTTI descriptor at `0x009022a0`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x007cf578`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 */
class PlaybackModeMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 105.
     *
     * @return The message.
     * @ghidraAddress 0x003d69a8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0011d578
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPlaybackModeMsgType.
     * @ghidraAddress 0x0011d5c8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlaybackModeMsg`.
     * @ghidraAddress 0x0011d5d8
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that PlaybackModeMsg::Type() reports.
 *
 * This word belongs to PlaybackModeMsg because PlaybackModeMsg::Type() at `0x0011d5c8` returns it,
 * and the registration at `0x003d9818` passes the same value, 105, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0144
 */
extern int g_nPlaybackModeMsgType;
