#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `13LoopToggleMsg` in the RTTI descriptor at `0x008f0850`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x007cfff0`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public
 * because Overlay::OnLoopToggle() at `0x0041f708` reads them directly with no accessor in the
 * image. It compares mPlayer with HudTrack::mPlayer and picks the displayed text on mOn.
 */
class LoopToggleMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 313.
     *
     * @return The message.
     * @ghidraAddress 0x003d7198
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001224f0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nLoopToggleMsgType.
     * @ghidraAddress 0x00122540
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `LoopToggleMsg`.
     * @ghidraAddress 0x00122550
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
 * @ghidraAddress 0x006d0254
 */
extern int g_nLoopToggleMsgType;
