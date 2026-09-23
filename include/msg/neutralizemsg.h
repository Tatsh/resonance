#pragma once

#include <iostream>

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `13NeutralizeMsg` in the RTTI descriptor at `0x008eecb8`, with CmdMsg as its one base. The
 * object is 0x14 bytes and its vtable is at `0x008122b8`. The word at `+0x04` belongs to CmdMsg.
 *
 * NeutralizePowerup builds the message at `0x001c9c64` from the three arguments of its slot 2,
 * storing the second at `+0x08`, the first at `+0x0c`, and the player at `+0x10`. Print() writes
 * only that player's colour name. All three members are public because
 * PhraseNeutralizer::PostTrackNeutralizedMsg() at `0x001c0980` reads them directly with no accessor
 * in the image. It compares mTrack with its own track, neutralises the four bars after mBar
 * (mBar + 1 through mBar + 4), and copies mPlayer into the DeployedPowerupMsg it sends.
 *
 * The destructor at `0x003e0288` is compiler-generated and has no declaration here.
 */
class NeutralizeMsg : public CmdMsg {
public:
    /**
     * Produce a message with a zero result on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress 0x003d7768
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e03a8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nNeutralizeMsgType.
     * @ghidraAddress 0x003e0418
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `NeutralizeMsg`.
     * @ghidraAddress 0x003e0428
     */
    virtual const char *Name();

    /**
     * Write the player's colour name to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3ea8
     */
    virtual void Print(std::ostream &stream);

    int mBar;        /*!< The bar the four neutralised bars follow. +0x08 */
    int mTrack;      /*!< The track to neutralise. +0x0c */
    Player *mPlayer; /*!< The player who deployed the neutraliser. +0x10 */
};

/**
 * Identity that NeutralizeMsg::Type() reports.
 *
 * This word belongs to NeutralizeMsg because NeutralizeMsg::Type() at `0x003e0418` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d031c
 */
extern int g_nNeutralizeMsgType;
