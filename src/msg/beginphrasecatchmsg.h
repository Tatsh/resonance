#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008f09b0`. It has Message as its one base. The object is 0x10 bytes.
 * New(), Clone(), and every stack build install the vtable at `0x007ddb08`. An identical table at
 * `0x007e0a70` has no reference in the image. The allocation in New() and the allocation in Clone()
 * report the same size.
 *
 * The destructor at `0x0019d738` is compiler-generated and has no declaration here. The routines
 * at `0x001b0f40`, `0x001d1b78`, and `0x003de7a8` are further emissions of Clone() in other
 * translation units, each allocating 0x10 bytes under the `MSG` tag and installing the same table.
 *
 * The payload layout comes from the run of field copies in Clone(). Every member is public because
 * Overlay::OnBeginPhraseCatch() at `0x004201c0` reads it directly with no accessor in the image.
 * It compares mPlayer with HudTrack::mPlayer, formats a non-zero mPoints with `%d`, and formats
 * mMultiplier with `x%d`.
 */
class BeginPhraseCatchMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    BeginPhraseCatchMsg() {
    }

    /**
     * Report the start of a phrase catch.
     *
     * Inline, with no address of its own. Catcher::CatchGem() at `0x001ac1c0` and the Scratcher
     * member at `0x001d0530` expand it on their stacks. The three arguments are the three members
     * in declaration order.
     *
     * @param pPlayer The player starting the phrase.
     * @param nPoints The points the phrase is worth.
     * @param nMultiplier The multiplier the phrase is caught under.
     */
    BeginPhraseCatchMsg(Player *pPlayer, int nPoints, int nMultiplier)
        : mPlayer(pPlayer), mPoints(nPoints), mMultiplier(nMultiplier) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 320.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7328
     * @ghidraAddress PAL: 0x0040f228
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x0019d7e8
     * @ghidraAddress PAL: 0x001a3550
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nBeginPhraseCatchMsgType.
     * @ghidraAddress NTSC-U/C: 0x0019d840
     * @ghidraAddress PAL: 0x001a35a8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `BeginPhraseCatchMsg`.
     * @ghidraAddress NTSC-U/C: 0x0019d850
     * @ghidraAddress PAL: 0x001a35b8
     */
    virtual const char *GetName() const;

    Player *mPlayer; /*!< The player starting the phrase. +0x04 */
    int mPoints;     /*!< The points the phrase is worth. +0x08 */
    int mMultiplier; /*!< The multiplier the phrase is caught under. +0x0c */
};

/**
 * Identity that BeginPhraseCatchMsg::Type() reports.
 *
 * This word belongs to BeginPhraseCatchMsg because BeginPhraseCatchMsg::Type() at `0x0019d840`
 * returns it, and the registration at `0x003d9818` passes the same value, 320, as the identity of
 * this class's factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d028c
 * @ghidraAddress PAL: 0x00713a24
 */
extern int g_nBeginPhraseCatchMsgType;
