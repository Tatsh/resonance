#pragma once

#include "msg/musemsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `7NoteMsg` in the RTTI descriptor at `0x008f0000`, with MuseMsg as its one base. The object is
 * 0x10 bytes and its vtable is at `0x00812e90`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable. The fields through `+0x08` belong to MuseMsg and are declared there.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e3760`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class NoteMsg : public MuseMsg {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dc208
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nNoteMsgType.
     * @ghidraAddress 0x003dc280
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `NoteMsg`.
     * @ghidraAddress 0x003dc290
     */
    virtual const char *Name();

private:
    unsigned char mUnknown09; // +0x09
    unsigned char mUnknown0a; // +0x0a
    int mUnknown0c;           // +0x0c
};

/**
 * Identity that NoteMsg::Type() reports.
 *
 * @ghidraAddress 0x006d01cc
 */
extern int g_nNoteMsgType;
