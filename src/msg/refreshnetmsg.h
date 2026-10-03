#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901d20`. It has Message as its one base. The object is 0x10 bytes
 * and its vtable is at `0x00812810`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). PrintExtra() labels `+0x0c` as
 * a track number and `+0x04` through `+0x08` as a range of bars, and PhraseMgr::OnRefreshNet()
 * reads all three directly.
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
     * @ghidraAddress NTSC-U/C: 0x003d72f0
     * @ghidraAddress PAL: 0x0040f1f0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003de630
     * @ghidraAddress PAL: 0x00416a88
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRefreshNetMsgType.
     * @ghidraAddress NTSC-U/C: 0x003de688
     * @ghidraAddress PAL: 0x00416ae0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RefreshNetMsg`.
     * @ghidraAddress NTSC-U/C: 0x003de698
     * @ghidraAddress PAL: 0x00416af0
     */
    virtual const char *GetName() const;

    /**
     * Write `tr#`, the word at `+0x0c`, ` bars `, and the two words at `+0x04` and `+0x08` joined
     * by ` - ` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3e08
     * @ghidraAddress PAL: 0x0041bfd8
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * The first bar of the range, written by PrintExtra() after ` bars `. +0x04
     *
     * PhraseMgr::OnRefreshNet() at `0x001ba928` starts its walk there.
     */
    int mFirstBar;

    /**
     * The bar one past the last of the range, written by PrintExtra() after ` - `. +0x08
     *
     * PhraseMgr::OnRefreshNet() stops its walk there.
     */
    int mEndBar;

    /**
     * The track, written by PrintExtra() after `tr#`. +0x0c
     *
     * PhraseMgr::OnRefreshNet() compares it with the track the manager serves.
     */
    int mTrack;
};

/**
 * Identity that RefreshNetMsg::Type() reports.
 *
 * This word belongs to RefreshNetMsg because RefreshNetMsg::Type() at `0x003de688` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0284
 * @ghidraAddress PAL: 0x00713a1c
 */
extern int g_nRefreshNetMsgType;
