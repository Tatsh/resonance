#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `17PhraseCapturedMsg` in the RTTI descriptor at `0x008ef830`, with Message as its one base. The
 * object is 0x28 bytes and its vtable is at `0x008126f0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003d8448`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class PhraseCapturedMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003debe0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPhraseCapturedMsgType.
     * @ghidraAddress 0x003dec68
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PhraseCapturedMsg`.
     * @ghidraAddress 0x003dec78
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14
    int mUnknown18; // +0x18
    int mUnknown1c; // +0x1c
    int mUnknown20; // +0x20
    int mUnknown24; // +0x24
};

/**
 * Identity that PhraseCapturedMsg::Type() reports.
 *
 * @ghidraAddress 0x006d02a4
 */
extern int g_nPhraseCapturedMsgType;
