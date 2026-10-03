#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901de0`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x007ddb98`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public because
 * AppTunnel::HandleMessage() at `0x00449938` reads them directly with no accessor in the image. It
 * scales mBar by the 1920 ticks of a bar and loads mTrack with `lb`, which reads only the low byte
 * of the word.
 *
 * The destructor at `0x0019d4e0` is compiler-generated and has no declaration here.
 */
class ClearGemsMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 403.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7480
     * @ghidraAddress PAL: 0x0040f380
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x0019d590
     * @ghidraAddress PAL: 0x001a32f8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nClearGemsMsgType.
     * @ghidraAddress NTSC-U/C: 0x0019d5e0
     * @ghidraAddress PAL: 0x001a3348
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ClearGemsMsg`.
     * @ghidraAddress NTSC-U/C: 0x0019d5f0
     * @ghidraAddress PAL: 0x001a3358
     */
    virtual const char *GetName() const;

    int mBar;   /*!< The bar to clear. +0x04 */
    int mTrack; /*!< The track. +0x08 */
};

/**
 * Identity that ClearGemsMsg::Type() reports.
 *
 * This word belongs to ClearGemsMsg because ClearGemsMsg::Type() at `0x0019d5e0` returns it, and
 * the registration at `0x003d9818` passes the same value, 403, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d02bc
 * @ghidraAddress PAL: 0x00713a54
 */
extern int g_nClearGemsMsgType;
