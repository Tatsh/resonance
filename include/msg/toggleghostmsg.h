#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `14ToggleGhostMsg` in the RTTI descriptor at `0x008ef0f0`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x007cf4e8`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public, the
 * player because LocalPlayer::Slot22() writes it and the flag because Overlay reads it.
 *
 * The destructor at `0x0011d710` is compiler-generated and has no declaration here. The routine
 * at `0x0011d748` is a further emission of the type-information accessor.
 */
class ToggleGhostMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 115.
     *
     * @return The message.
     * @ghidraAddress 0x003d6c28
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0011d7c0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nToggleGhostMsgType.
     * @ghidraAddress 0x0011d810
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ToggleGhostMsg`.
     * @ghidraAddress 0x0011d820
     */
    virtual const char *Name();

public:
    /**
     * Player the message is about.
     *
     * LocalPlayer::Slot22() at `0x0011e908` writes the player here before sending, which is what
     * types this member as a player rather than as a payload word.
     *
     * +0x04
     */
    Player *mUnknown04;

    /**
     * Non-zero to light the player's track display, zero to darken it.
     *
     * LocalPlayer::Slot22() at `0x0011e908` writes its argument here. Overlay::OnToggleGhost() at
     * `0x0042aef8` reads it directly with no accessor in the image and selects one of two values
     * from the track display for each of the display's kind-11 components.
     *
     * +0x08
     */
    int mOn;
};

/**
 * Identity that ToggleGhostMsg::Type() reports.
 *
 * This word belongs to ToggleGhostMsg because ToggleGhostMsg::Type() at `0x0011d810` returns it,
 * and the registration at `0x003d9818` passes the same value, 115, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0194
 */
extern int g_nToggleGhostMsgType;
