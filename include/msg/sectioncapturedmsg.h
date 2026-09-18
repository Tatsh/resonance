#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18SectionCapturedMsg` in the RTTI descriptor at `0x008efcf0`, with Message as its one base.
 * The object is 0x18 bytes and its vtable is at `0x008126a8`. The members below are the whole of
 * the class: everything recovered comes from them, and no other routine in the image refers to
 * this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003d8578`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SectionCapturedMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dee18
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nSectionCapturedMsgType.
     * @ghidraAddress 0x003dee80
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `SectionCapturedMsg`.
     * @ghidraAddress 0x003dee90
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14
};

/**
 * Identity that SectionCapturedMsg::Type() reports.
 *
 * @ghidraAddress 0x006d02ac
 */
extern int g_nSectionCapturedMsgType;
