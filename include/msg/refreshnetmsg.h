#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `13RefreshNetMsg` in the RTTI descriptor at `0x00901d20`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x00812810`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Print() labels `+0x0c` as a track number and `+0x04` through `+0x08` as a range of bars.
 *
 * The destructor at `0x003de540` is compiler-generated and has no declaration here.
 */
class RefreshNetMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d72f0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003de630
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRefreshNetMsgType.
     * @ghidraAddress 0x003de688
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RefreshNetMsg`.
     * @ghidraAddress 0x003de698
     */
    virtual const char *Name();

    /**
     * Write `tr#`, the word at `+0x0c`, ` bars `, and the two words at `+0x04` and `+0x08` joined
     * by ` - ` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3e08
     */
    virtual void Print(std::ostream &stream);

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that RefreshNetMsg::Type() reports.
 *
 * This word belongs to RefreshNetMsg because RefreshNetMsg::Type() at `0x003de688` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0284
 */
extern int g_nRefreshNetMsgType;
