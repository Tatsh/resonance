#pragma once

#include "app/hudutil.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef340`. It has Message as its one base. The object is 0xc bytes
 * and its vtable is at `0x007e45b8`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public
 * because Overlay::OnPowerupFailed() at `0x0041f138` reads them directly with no accessor in the
 * image. It compares mPlayer with HudTrack::mPlayer and switches over mKind across the five
 * powerup kinds.
 *
 * The destructor at `0x001ca6c8` is compiler-generated and has no declaration here.
 */
class PowerupFailedMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 307.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7038
     * @ghidraAddress PAL: 0x0040ef28
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001ca778
     * @ghidraAddress PAL: 0x001d0630
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPowerupFailedMsgType.
     * @ghidraAddress NTSC-U/C: 0x001ca7c8
     * @ghidraAddress PAL: 0x001d0680
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PowerupFailedMsg`.
     * @ghidraAddress NTSC-U/C: 0x001ca7d8
     * @ghidraAddress PAL: 0x001d0690
     */
    virtual const char *GetName() const;

    PowerupType mKind; /*!< The powerup that failed. +0x04 */
    Player *mPlayer;   /*!< The player who deployed it. +0x08 */
};

/**
 * Identity that PowerupFailedMsg::Type() reports.
 *
 * This word belongs to PowerupFailedMsg because PowerupFailedMsg::Type() at `0x001ca7c8` returns
 * it, and the registration at `0x003d9818` passes the same value, 307, as the identity of this
 * class's factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0224
 * @ghidraAddress PAL: 0x007139bc
 */
extern int g_nPowerupFailedMsgType;
