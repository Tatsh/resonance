#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `9SeekerMsg` in the RTTI descriptor at `0x008ef800`, with Message as its one base. The object
 * is 0x1c bytes and its vtable is at `0x00812cd8`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003d81f0`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class SeekerMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dcce0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nSeekerMsgType.
     * @ghidraAddress 0x003dcd50
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `SeekerMsg`.
     * @ghidraAddress 0x003dcd60
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14
    int mUnknown18; // +0x18
};

/**
 * Identity that SeekerMsg::Type() reports.
 *
 * This word belongs to SeekerMsg because SeekerMsg::Type() at `0x003dcd50` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d01fc
 */
extern int g_nSeekerMsgType;
