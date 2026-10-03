#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008f07b0`. It has Message as its one base. The object is 0x14 bytes
 * and its vtable is at `0x007e4528`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). Every field is public, because
 * JamEffectsMgr::PostRemixFxMsg() at `0x001a54d8` reads all four directly and the image has no
 * accessor for any of them.
 *
 * The destructor at `0x001ca950` is compiler-generated and has no declaration here.
 */
class JamEffectMsg : public Message {
public:
    /**
     * Identity that Type() reports, 417.
     *
     * @ghidraAddress NTSC-U/C: 0x006d032c
     * @ghidraAddress PAL: 0x00713ac4
     */
    static int sID;

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 417.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d77e0
     * @ghidraAddress PAL: 0x0040f6e0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001caa00
     * @ghidraAddress PAL: 0x001d08b8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x001caa60
     * @ghidraAddress PAL: 0x001d0918
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `JamEffectMsg`.
     * @ghidraAddress NTSC-U/C: 0x001caa70
     * @ghidraAddress PAL: 0x001d0928
     */
    virtual const char *GetName() const;

    int mBar;    /*!< The bar the effect toggles on. +0x04 */
    int mTrack;  /*!< The track, which PostRemixFxMsg() compares with its manager's. +0x08 */
    int mEffect; /*!< The effect type, the bit PostRemixFxMsg() toggles in the bar's mask. +0x0c */
    /** The deploying player, which PostRemixFxMsg() copies into RemixFXMsg::mPlayer. +0x10 */
    Player *mPlayer;
};
