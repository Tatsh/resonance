#pragma once

#include "app/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11TracksOnMsg` in the RTTI descriptor at `0x00901e40`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x00812780`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e4408`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class TracksOnMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003de910
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_dwMsgIdSongSelect.
     * @ghidraAddress 0x003de960
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `TracksOnMsg`.
     * @ghidraAddress 0x003de970
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that TracksOnMsg::Type() reports.
 *
 * This word was already titled `g_dwMsgIdSongSelect` in the program by another subsystem, and the
 * reconstruction follows that name rather than imposing its own. The two disagree:
 * TracksOnMsg::Type() is the only routine that reads the word, so the existing title describes a
 * handler that compares against it rather than the class that reports it, and it is likely wrong.
 *
 * @ghidraAddress 0x006d0294
 */
extern unsigned int g_dwMsgIdSongSelect;
