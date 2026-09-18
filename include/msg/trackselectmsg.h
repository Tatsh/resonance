#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `14TrackSelectMsg` in the RTTI descriptor at `0x00902a20`, with Message as its one base. The
 * object is 0x14 bytes and its vtable is at `0x00812d68`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e3ae8`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class TrackSelectMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dc930
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwTrackSelectMsgType.
     * @ghidraAddress 0x003dc990
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `TrackSelectMsg`.
     * @ghidraAddress 0x003dc9a0
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
    int mUnknown10; // +0x10
};

/**
 * Identity that TrackSelectMsg::Type() reports.
 *
 * This word belongs to TrackSelectMsg because TrackSelectMsg::Type() at `0x003dc990` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d01ec
 */
extern unsigned int g_dwTrackSelectMsgType;
