#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `11FadeGameMsg` in the RTTI descriptor at `0x008ef3e0`, with Message as its one base. The object
 * is 0xc bytes and its vtable is at `0x007dc430`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Both members are public
 * because Overlay::OnFadeGame() at `0x0042b1f8` reads them directly with no accessor in the image.
 * It passes both to HudScreenFlash::Start(), converting mDuration to a float, and hides the
 * panel's message text when mFadeIn is clear.
 */
class FadeGameMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 424.
     *
     * @return The message.
     * @ghidraAddress 0x003d7978
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00193f88
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nFadeGameMsgType.
     * @ghidraAddress 0x00193fd8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `FadeGameMsg`.
     * @ghidraAddress 0x00193fe8
     */
    virtual const char *Name();

    int mDuration; /*!< The length of the fade in milliseconds. +0x04 */
    int mFadeIn;   /*!< Non-zero to fade the game in, zero to fade it out. +0x08 */
};

/**
 * Identity that FadeGameMsg::Type() reports.
 *
 * This word belongs to FadeGameMsg because FadeGameMsg::Type() at `0x00193fd8` returns it, and the
 * registration at `0x003d9818` passes the same value, 424, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0364
 */
extern int g_nFadeGameMsgType;
