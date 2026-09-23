#pragma once

#include <iostream>

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12AutoCatchMsg` in the RTTI descriptor at `0x008f0010`, with CmdMsg as its one base. The
 * object is 0x14 bytes and its vtable is at `0x00811c88`. The word at `+0x04` belongs to CmdMsg.
 *
 * Print() labels `+0x0c` as a track number and writes the identifier of the player at `+0x10`
 * after ` p#`, followed by the bar at `+0x08`.
 *
 * The three members are public because code outside the class accesses them directly with no
 * accessor in the image. AutocatchPowerup::Slot2() at `0x001c9628` writes all three into a stack
 * message. Catcher::OnAutoCatch() at `0x001ac688` compares mTrack with its own track, plays mBar
 * for mPlayer, and sets CmdMsg::mUnknown04 to mark the message handled.
 *
 * The destructor at `0x003e2460` is compiler-generated and has no declaration here.
 */
class AutoCatchMsg : public CmdMsg {
public:
    /**
     * Produce a message with a zero result on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d7c68
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e2580
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAutoCatchMsgType.
     * @ghidraAddress 0x003e25f0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AutoCatchMsg`.
     * @ghidraAddress 0x003e2600
     */
    virtual const char *Name();

    /**
     * Write `tr#`, the track, ` p#`, the player's identifier, a space, and the word at `+0x08` to
     * a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e4208
     */
    virtual void Print(std::ostream &stream);

    int mBar;        /*!< The bar to catch automatically. +0x08 */
    int mTrack;      /*!< The track the bar lies on. +0x0c */
    Player *mPlayer; /*!< The player the catch is for. +0x10 */
};

/**
 * Identity that AutoCatchMsg::Type() reports.
 *
 * This word belongs to AutoCatchMsg because AutoCatchMsg::Type() at `0x003e25f0` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d03cc
 */
extern int g_nAutoCatchMsgType;
