#pragma once

#include <iostream>

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `26PlayersTrackNeutralizedMsg` in the RTTI descriptor at `0x008eed28`, with CmdMsg as its one
 * base. The object is 0x10 bytes and its vtable is at `0x00812270`. The word at `+0x04` belongs
 * to CmdMsg.
 *
 * PhraseNeutralizer::PostTrackNeutralizedMsg() builds the message at `0x001c0b2c`, once per
 * affected player, with a per-player total at `+0x08` and the player at `+0x0c`. Print() writes
 * only that player's colour name. New() zeroes both words.
 *
 * The destructor at `0x003e0490` is compiler-generated and has no declaration here.
 */
class PlayersTrackNeutralizedMsg : public CmdMsg {
public:
    /**
     * Produce a message with every word zeroed on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d77a0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e05b0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPlayersTrackNeutralizedMsgType.
     * @ghidraAddress 0x003e0618
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlayersTrackNeutralizedMsg`.
     * @ghidraAddress 0x003e0628
     */
    virtual const char *Name();

    /**
     * Write the player's colour name to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3f30
     */
    virtual void Print(std::ostream &stream);

private:
    int mUnknown08 = 0;        // +0x08
    Player *mPlayer = nullptr; // +0x0c
};

/**
 * Identity that PlayersTrackNeutralizedMsg::Type() reports.
 *
 * This word belongs to PlayersTrackNeutralizedMsg because PlayersTrackNeutralizedMsg::Type() at
 * `0x003e0618` returns it. Several handlers elsewhere read the same word to compare against it,
 * which is the expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0324
 */
extern int g_nPlayersTrackNeutralizedMsgType;
