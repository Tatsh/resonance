#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `13StreakOverMsg` in the RTTI descriptor at `0x008ef7d0`, with Message as its one base. The
 * object is 0x8 bytes and its vtable is at `0x00812f68`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003dbc48` is compiler-generated and has no declaration here.
 */
class StreakOverMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d6cd0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dbcf8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nStreakOverMsgType.
     * @ghidraAddress 0x003dbd40
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `StreakOverMsg`.
     * @ghidraAddress 0x003dbd50
     */
    virtual const char *Name();

private:
    int mUnknown04; // +0x04
};

/**
 * Identity that StreakOverMsg::Type() reports.
 *
 * This word belongs to StreakOverMsg because StreakOverMsg::Type() at `0x003dbd40` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d01ac
 */
extern int g_nStreakOverMsgType;
