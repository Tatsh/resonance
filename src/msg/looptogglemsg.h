#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008f0850`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x007cfff0`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public
 * because Overlay::OnLoopToggle() at `0x0041f708` reads them directly with no accessor in the
 * image. It compares mPlayer with HudTrack::mPlayer and picks the displayed text on mOn.
 *
 * The destructor at `0x00122440` is compiler-generated and has no declaration here.
 */
class LoopToggleMsg : public Message {
public:
    /**
     * Construct a message with every payload word indeterminate.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    LoopToggleMsg() {
    }

    /**
     * Report that a player started or stopped looping.
     *
     * Inline, with no address of its own. LocalPlayer's loop handler expands it on its stack at
     * `0x0011e7a4`.
     *
     * @param nOn Non-zero when looping starts.
     * @param pPlayer The player who toggled looping.
     */
    LoopToggleMsg(int nOn, Player *pPlayer) : mOn(nOn), mPlayer(pPlayer) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 313.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7198
     * @ghidraAddress PAL: 0x0040f098
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001224f0
     * @ghidraAddress PAL: 0x00122b08
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nLoopToggleMsgType.
     * @ghidraAddress NTSC-U/C: 0x00122540
     * @ghidraAddress PAL: 0x00122b58
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `LoopToggleMsg`.
     * @ghidraAddress NTSC-U/C: 0x00122550
     * @ghidraAddress PAL: 0x00122b68
     */
    virtual const char *Name();

    int mOn;         /*!< Non-zero when looping starts, zero when it stops. +0x04 */
    Player *mPlayer; /*!< The player who toggled looping. +0x08 */
};

/**
 * Identity that LoopToggleMsg::Type() reports.
 *
 * This word belongs to LoopToggleMsg because LoopToggleMsg::Type() at `0x00122540` returns it, and
 * the registration at `0x003d9818` passes the same value, 313, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0254
 * @ghidraAddress PAL: 0x007139ec
 */
extern int g_nLoopToggleMsgType;
