#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `16MetStartPauseMsg` in the RTTI descriptor at `0x00901e50`, with Message as its one base. The
 * object is 0x4 bytes and its vtable is at `0x00811bb0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e4488`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class MetStartPauseMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e2ac0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMetStartPauseMsgType.
     * @ghidraAddress 0x003e2af8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MetStartPauseMsg`.
     * @ghidraAddress 0x003e2b08
     */
    virtual const char *Name();
};

/**
 * Identity that MetStartPauseMsg::Type() reports.
 *
 * @ghidraAddress 0x006d03e4
 */
extern int g_nMetStartPauseMsgType;
