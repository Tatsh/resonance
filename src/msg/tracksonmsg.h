#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11TracksOnMsg` in the RTTI descriptor at `0x00901e40`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x00812780`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), and Print() labels `+0x04` as
 * a bar and `+0x08` as tracks. Both members are public because Mixer::OnTracksOn() at `0x001a76d0`
 * reads them directly with no accessor in the image.
 *
 * The destructor at `0x003de820` is compiler-generated and has no declaration here.
 */
class TracksOnMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    TracksOnMsg() {
    }

    /**
     * Report how many tracks are on from a bar.
     *
     * Inline, with no address of its own. Gamer's builds at `0x00111dc8` and `0x001122e4` expand
     * it on their stacks, the first with the count of tracks its loop found on and the second with
     * zero.
     *
     * @param nBar The bar.
     * @param nTracks The number of tracks on.
     */
    TracksOnMsg(int nBar, int nTracks) : mBar(nBar), mTracks(nTracks) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress 0x003d7360
     */
    static Message *New();

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
     * @return g_dwTracksOnMsgType.
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

    /**
     * Write `bar `, the word at `+0x04`, ` tracks `, and the word at `+0x08` to a diagnostic
     * stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e4408
     */
    virtual void Print(std::ostream &stream);

    int mBar;    /*!< The bar, labelled `bar` by Print(). +0x04 */
    int mTracks; /*!< The number of tracks on, labelled `tracks` by Print(). +0x08 */
};

/**
 * Identity that TracksOnMsg::Type() reports.
 *
 * This word belongs to TracksOnMsg because TracksOnMsg::Type() at `0x003de960` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0294
 */
extern unsigned int g_dwTracksOnMsgType;
