#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `14JuiceAmountMsg` in the RTTI descriptor at `0x008ef840`, with Message as its one base. The
 * object is 0xc bytes and its vtable is at `0x008121e0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003e41d8`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class JuiceAmountMsg : public Message {
public:
    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e08a8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nJuiceAmountMsgType.
     * @ghidraAddress 0x003e08f8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `JuiceAmountMsg`.
     * @ghidraAddress 0x003e0908
     */
    virtual const char *Name();

public:
    /**
     * Player the message is about.
     *
     * Player::Slot11() at `0x0012f788` writes the player here before sending, which is what types
     * this member as a player rather than as a payload word.
     *
     * +0x04
     */
    Player *mUnknown04;

    /** Written by the same site from a value clamped to a maximum of 800. +0x08 */
    int mUnknown08;
};

/**
 * Identity that JuiceAmountMsg::Type() reports.
 *
 * This word belongs to JuiceAmountMsg because JuiceAmountMsg::Type() at `0x003e08f8` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0334
 */
extern int g_nJuiceAmountMsgType;
