#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `20MetStartNetLaunchMsg` in the RTTI descriptor at `0x008eecd8`, with Message as its one base.
 * The object is 0x4 bytes and its vtable is at `0x00811bf8`. The members below are the whole of
 * the class: everything recovered comes from them, and no other routine in the image refers to
 * this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e4460`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class MetStartNetLaunchMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e2960
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMetStartNetLaunchMsgType.
     * @ghidraAddress 0x003e2998
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MetStartNetLaunchMsg`.
     * @ghidraAddress 0x003e29a8
     */
    virtual const char *Name();
};

/**
 * Identity that MetStartNetLaunchMsg::Type() reports.
 *
 * This word belongs to MetStartNetLaunchMsg because MetStartNetLaunchMsg::Type() at `0x003e2998`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d03dc
 */
extern int g_nMetStartNetLaunchMsgType;
