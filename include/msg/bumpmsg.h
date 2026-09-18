#pragma once

#include "msg/cmdmsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `7BumpMsg` in the RTTI descriptor at `0x008ef350`, with CmdMsg as its one base. The object is
 * 0x14 bytes and its vtable is at `0x00811fa0`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable. The fields through `+0x0f` belong to CmdMsg and are declared there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e3fb8`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class BumpMsg : public CmdMsg {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e13d0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nBumpMsgType.
     * @ghidraAddress 0x003e1440
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `BumpMsg`.
     * @ghidraAddress 0x003e1450
     */
    virtual const char *Name();

private:
    int mUnknown10; // +0x10
};

/**
 * Identity that BumpMsg::Type() reports.
 *
 * This word belongs to BumpMsg because BumpMsg::Type() at `0x003e1440` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0374
 */
extern int g_nBumpMsgType;
