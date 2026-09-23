#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `15NearestTrackMsg` in the RTTI descriptor at `0x008ef820`, with Message as its one base. The
 * object is 0x8 bytes and its vtable is at `0x00812ae0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x003dd6d0` is compiler-generated and has no declaration here.
 */
class NearestTrackMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d70a8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dd7c0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nNearestTrackMsgType.
     * @ghidraAddress 0x003dd808
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `NearestTrackMsg`.
     * @ghidraAddress 0x003dd818
     */
    virtual const char *Name();

    /**
     * Write the word to a diagnostic stream, or `reset` when it is -1.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3d50
     */
    virtual void Print(std::ostream &stream);

private:
    int mUnknown04; // +0x04
};

/**
 * Identity that NearestTrackMsg::Type() reports.
 *
 * This word belongs to NearestTrackMsg because NearestTrackMsg::Type() at `0x003dd808` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0234
 */
extern int g_nNearestTrackMsgType;
