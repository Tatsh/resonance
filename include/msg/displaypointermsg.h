#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `17DisplayPointerMsg` in the RTTI descriptor at `0x00901e30`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x00812a98`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The class overrides Message::Print() at `0x003d8358`. That body streams the payload and is not
 * recovered, so the override is recorded here rather than declared.
 */
class DisplayPointerMsg : public Message {
public:
    /**
     * Report where a player's powerup pointer rests.
     *
     * No address attaches to the constructor. GamePowerupPlacer builds the message on its own
     * stack at `0x001ccd64`, `0x001ccdac`, `0x001cce04`, `0x001ccec8`, `0x001ccfc0`, and
     * `0x001cd0ec` and hands the address to MsgSource::Send(). The compiler expands the
     * constructor into each of the six sites. The three arguments are the three members in
     * declaration order.
     *
     * @param nBar The bar the pointer rests on.
     * @param nPlayerValue Whatever Player::Slot4() reports for the player.
     * @param pPlayer The player whose pointer moved.
     */
    DisplayPointerMsg(int nBar, int nPlayerValue, Player *pPlayer);

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003dd980
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nDisplayPointerMsgType.
     * @ghidraAddress 0x003dd9d8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `DisplayPointerMsg`.
     * @ghidraAddress 0x003dd9e8
     */
    virtual const char *Name();

private:
    // The three names come from GamePowerupPlacer, the one producer of the message, which writes
    // the bar its cursor rests on, whatever Player::Slot4() reports, and the player itself.
    int mBar;         // +0x04
    int mPlayerValue; // +0x08
    Player *mPlayer;  // +0x0c
};

/**
 * Identity that DisplayPointerMsg::Type() reports.
 *
 * This word belongs to DisplayPointerMsg because DisplayPointerMsg::Type() at `0x003dd9d8`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d023c
 */
extern int g_nDisplayPointerMsgType;
