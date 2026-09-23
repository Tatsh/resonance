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
 * The payload layout comes from the run of field copies in Clone(). Print() labels mTrack as a
 * track number and mFirstBar through mEndBar as a range of song bars. The end is one past the
 * last bar, because the stack build at `0x00101e6c` invalidates a single bar as that bar and the
 * bar after it.
 *
 * Every member is public because PhraseMgr::OnInvalidateTrack() at `0x001c0110` reads all three
 * directly with no accessor in the image.
 *
 * The destructor at `0x003de360` is compiler-generated and has no declaration here.
 */
class InvalidateTrackMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    InvalidateTrackMsg() {
    }

    /**
     * Invalidate a range of song bars on a track.
     *
     * Inline, with no address of its own. The two stack builds at `0x00101ce0` and `0x00101e6c`
     * expand it. The three arguments are the three members in declaration order.
     *
     * @param nFirstBar The first song bar of the range.
     * @param nEndBar The song bar one past the last of the range.
     * @param nTrack The track.
     */
    InvalidateTrackMsg(int nFirstBar, int nEndBar, int nTrack)
        : mFirstBar(nFirstBar), mEndBar(nEndBar), mTrack(nTrack) {
    }

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

    int mFirstBar; /*!< The first song bar of the range. +0x04 */
    int mEndBar;   /*!< The song bar one past the last of the range. +0x08 */
    int mTrack;    /*!< The track. +0x0c */
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
