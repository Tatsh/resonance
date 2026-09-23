#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18InvalidateTrackMsg` in the RTTI descriptor at `0x008ef400`, with Message as its one base.
 * The object is 0x10 bytes and its vtable is at `0x00812858`. The members below are the whole of
 * the class: everything recovered comes from them, and no other routine in the image refers to
 * this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Print() labels `+0x0c` as a track number and `+0x04` through `+0x08` as a range of song bars.
 *
 * The destructor at `0x003de360` is compiler-generated and has no declaration here.
 */
class InvalidateTrackMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d72b8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003de450
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nInvalidateTrackMsgType.
     * @ghidraAddress 0x003de4a8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `InvalidateTrackMsg`.
     * @ghidraAddress 0x003de4b8
     */
    virtual const char *Name();

    /**
     * Write `tr#`, the word at `+0x0c`, ` song-bars `, and the two words at `+0x04` and `+0x08`
     * joined by `-` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3d90
     */
    virtual void Print(std::ostream &stream);

private:
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that InvalidateTrackMsg::Type() reports.
 *
 * This word belongs to InvalidateTrackMsg because InvalidateTrackMsg::Type() at `0x003de4a8`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d027c
 */
extern int g_nInvalidateTrackMsgType;
