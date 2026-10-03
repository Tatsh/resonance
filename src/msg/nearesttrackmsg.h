#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef820`. It has Message as its one base. The object is 0x8 bytes
 * and its vtable is at `0x00812ae0`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
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
     * @ghidraAddress NTSC-U/C: 0x003d70a8
     * @ghidraAddress PAL: 0x0040ef98
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dd7c0
     * @ghidraAddress PAL: 0x00415bf8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nNearestTrackMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dd808
     * @ghidraAddress PAL: 0x00415c40
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `NearestTrackMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dd818
     * @ghidraAddress PAL: 0x00415c50
     */
    virtual const char *GetName() const;

    /**
     * Write the word to a diagnostic stream, or `reset` when it is -1.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3d50
     * @ghidraAddress PAL: 0x0041bf20
     */
    virtual void PrintExtra(std::ostream &stream) const;

private:
    int mTrack; // +0x04, or -1 (written as `reset` by PrintExtra())
};

/**
 * Identity that NearestTrackMsg::Type() reports.
 *
 * This word belongs to NearestTrackMsg because NearestTrackMsg::Type() at `0x003dd808` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0234
 * @ghidraAddress PAL: 0x007139cc
 */
extern int g_nNearestTrackMsgType;
