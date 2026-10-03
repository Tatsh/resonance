#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef3d0`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x007ce720`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). The word at `+0x04` is a bar
 * and the word at `+0x08` a track, which the builds and every reader agree on. The readers compare
 * the track with their own and move their seeker to the bar.
 *
 * The destructor at `0x00116000` is compiler-generated and has no declaration here. The routine
 * at `0x00116038` is a further emission of the type-information accessor.
 */
class InvalidateSeekerMsg : public Message {
public:
    /**
     * Identity that Type() reports, 317.
     *
     * @ghidraAddress NTSC-U/C: 0x006d0274
     * @ghidraAddress PAL: 0x00713a0c
     */
    static int sID;

    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    InvalidateSeekerMsg() {
    }

    /**
     * Invalidate the seeker of one track from a bar on.
     *
     * Inline, with no address of its own. Gamer's builds at `0x00111168` and `0x00111820` expand
     * it on their stacks.
     *
     * @param nBar The bar.
     * @param nTrack The track.
     */
    InvalidateSeekerMsg(int nBar, int nTrack) : mBar(nBar), mTrack(nTrack) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 317.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7280
     * @ghidraAddress PAL: 0x0040f180
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001160b0
     * @ghidraAddress PAL: 0x00116558
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x00116100
     * @ghidraAddress PAL: 0x001165a8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `InvalidateSeekerMsg`.
     * @ghidraAddress NTSC-U/C: 0x00116110
     * @ghidraAddress PAL: 0x001165b8
     */
    virtual const char *GetName() const;

public:
    // Public because Voxer::DispatchPriv(), Scratcher::DispatchPriv(), and
    // NotePitcher::DispatchPriv() read the members below directly, through an InvalidateSeekerMsg
    // pointer from outside the hierarchy, and the image exposes no accessor. A friend declaration
    // fits equally well.
    int mBar;   // +0x04
    int mTrack; // +0x08
};
