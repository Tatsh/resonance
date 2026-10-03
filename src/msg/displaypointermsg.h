#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901e30`. It has Message as its one base. The object is 0x10 bytes
 * and its vtable is at `0x00812a98`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not.
 *
 * Print() labels mPlayerValue as a track number, `tr# `, and writes `remove` in its place when it
 * is -1.
 *
 * The destructor at `0x003dd890` is compiler-generated and has no declaration here.
 */
class DisplayPointerMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it and stores only the vtable pointer. A declaration is required
     * because the class declares a second constructor.
     */
    DisplayPointerMsg() {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d70e0
     * @ghidraAddress PAL: 0x0040efd0
     */
    static Message *New();

    /**
     * Report where a player's powerup pointer rests.
     *
     * No address attaches to the constructor. GamePowerupPlacer builds the message on its own
     * stack at six sites and hands the address to MsgSource::Send(). Four of them, at
     * `0x001ccd64`, `0x001cce04`, `0x001ccec8`, and `0x001cd0ec`, expand this constructor. The
     * two remove sites, at `0x001ccdac` and `0x001ccfc0`, store only mPlayerValue and mPlayer
     * into a default-constructed message. The three arguments are the three members in
     * declaration order.
     *
     * @param nBar The bar the pointer rests on.
     * @param nPlayerValue Whatever Player::GetTrack() reports for the player.
     * @param pPlayer The player whose pointer moved.
     */
    DisplayPointerMsg(int nBar, int nPlayerValue, Player *pPlayer);

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dd980
     * @ghidraAddress PAL: 0x00415db8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nDisplayPointerMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dd9d8
     * @ghidraAddress PAL: 0x00415e10
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `DisplayPointerMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dd9e8
     * @ghidraAddress PAL: 0x00415e20
     */
    virtual const char *Name();

    /**
     * Write `remove` when mPlayerValue is -1, and otherwise `tr# `, mPlayerValue, `:`, the bar, a
     * space, and the player's colour name, to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003d8358
     * @ghidraAddress PAL: 0x004106f0
     */
    virtual void Print(std::ostream &stream);

    // The three names come from GamePowerupPlacer, the one producer of the message, which writes
    // the bar its cursor rests on, whatever Player::GetTrack() reports, and the player itself. The
    // members are public because its two remove sites, 0x001ccdb8 in MoveCursor() and the one at
    // 0x001ccfcc in DeployPowerup(), store mPlayerValue and mPlayer directly and do not
    // set mBar.
    int mBar;         /*!< The bar the pointer rests on. +0x04 */
    int mPlayerValue; /*!< Player::GetTrack() of the player, or -1 to remove the pointer. +0x08 */
    Player *mPlayer;  /*!< The player whose pointer moved. +0x0c */
};

/**
 * Identity that DisplayPointerMsg::Type() reports.
 *
 * This word belongs to DisplayPointerMsg because DisplayPointerMsg::Type() at `0x003dd9d8`
 * returns it. Several handlers elsewhere read the same word to compare against it, which is the
 * expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d023c
 * @ghidraAddress PAL: 0x007139d4
 */
extern int g_nDisplayPointerMsgType;
