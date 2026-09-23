#pragma once

#include <iostream>

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `10CrippleMsg` in the RTTI descriptor at `0x00901b30`, with CmdMsg as its one base. The object
 * is 0x14 bytes and its vtable is at `0x00811c40`. The word at `+0x04` belongs to CmdMsg.
 *
 * CripplePowerup builds the message at `0x001c9854` from the three arguments of its slot 2,
 * storing the player at `+0x08`, the first argument at `+0x0c`, and the second at `+0x10`, and
 * reads the result word back after sending. Print() labels `+0x0c` as a track number and writes
 * the player's identifier after ` p#`.
 *
 * mPlayer and mTrack are public because Gamer's crippler handler at `0x00111230` reads them
 * directly with no accessor in the image, and sets CmdMsg::mUnknown04 to 1 when victims exist.
 *
 * The destructor at `0x003e2668` is compiler-generated and has no declaration here.
 */
class CrippleMsg : public CmdMsg {
public:
    /**
     * Produce a message with a zero result on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d7ca0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e2788
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nCrippleMsgType.
     * @ghidraAddress 0x003e27f8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `CrippleMsg`.
     * @ghidraAddress 0x003e2808
     */
    virtual const char *Name();

    /**
     * Write `tr#`, the track, ` p#`, and the player's identifier to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e4290
     */
    virtual void Print(std::ostream &stream);

    Player *mPlayer; /*!< The player who deployed the crippler. +0x08 */
    int mTrack;      /*!< The track, labelled by Print(). +0x0c */
    /**
     * The bar the crippler is deployed at. +0x10
     *
     * Public because CripplePowerup::Deploy() at `0x001c9830` stores its bar argument here
     * directly, and the image has no accessor.
     */
    int mBar;
};

/**
 * Identity that CrippleMsg::Type() reports.
 *
 * This word belongs to CrippleMsg because CrippleMsg::Type() at `0x003e27f8` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d03d4
 */
extern int g_nCrippleMsgType;
