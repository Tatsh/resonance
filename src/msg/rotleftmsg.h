#pragma once

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eec58`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x007cf608`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). The types come from
 * InputMap::OnControllerReading(), the one builder, which stores the resolved player and the
 * controller reading's position. TrackSelector::DispatchPriv() reads both fields directly, so
 * they are public.
 *
 * The destructor at `0x0011d288` is compiler-generated and has no declaration here. The routine
 * at `0x0011d2c0` is a further emission of the type-information accessor.
 */
class RotLeftMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the player unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    RotLeftMsg() {
    }

    /**
     * Report a rotate-left press.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x001197d8` expands
     * it on its stack.
     *
     * @param pPlayer The player the controller belongs to.
     * @param position The song position of the reading.
     */
    RotLeftMsg(Player *pPlayer, Mid::MBT position) : mPlayer(pPlayer), mPosition(position) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 101.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d68a8
     * @ghidraAddress PAL: 0x0040e798
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x0011d338
     * @ghidraAddress PAL: 0x0011d8c0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRotLeftMsgType.
     * @ghidraAddress NTSC-U/C: 0x0011d388
     * @ghidraAddress PAL: 0x0011d910
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RotLeftMsg`.
     * @ghidraAddress NTSC-U/C: 0x0011d398
     * @ghidraAddress PAL: 0x0011d920
     */
    virtual const char *GetName() const;

public:
    /** The player to rotate. TrackSelector::DispatchPriv() reads it at `0x0013b8b0`. +0x04 */
    Player *mPlayer;

    /**
     * The song position of the rotation. TrackSelector::DispatchPriv() reads it as the payload
     * of the rebind. +0x08
     */
    Mid::MBT mPosition;
};

/**
 * Identity that RotLeftMsg::Type() reports.
 *
 * This word belongs to RotLeftMsg because RotLeftMsg::Type() at `0x0011d388` returns it, and the
 * registration at `0x003d9818` passes the same value, 101, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0124
 * @ghidraAddress PAL: 0x007138bc
 */
extern int g_nRotLeftMsgType;
