#pragma once

#include <iostream>

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eed28`. It has CmdMsg as its one base. The object is 0x10 bytes
 * and its vtable is at `0x00812270`. The word at `+0x04` belongs to CmdMsg.
 *
 * PhraseNeutralizer::PostTrackNeutralizedMsg() builds the message at `0x001c0b2c`, once per
 * affected player, with a per-player total at `+0x08` and the player at `+0x0c`. PrintExtra()
 * writes only that player's colour name. New() zeroes both words.
 *
 * Both members are public because Overlay::OnPlayersTrackNeutralized() at `0x00420588` reads them
 * directly with no accessor in the image. It compares mPlayer with HudTrack::mPlayer and formats
 * mPoints into `NEUTRALIZED!\n%d POINTS`.
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
     * @ghidraAddress NTSC-U/C: 0x003d77a0
     * @ghidraAddress PAL: 0x0040f6a0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e05b0
     * @ghidraAddress PAL: 0x00418a08
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPlayersTrackNeutralizedMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e0618
     * @ghidraAddress PAL: 0x00418a70
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PlayersTrackNeutralizedMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e0628
     * @ghidraAddress PAL: 0x00418a80
     */
    virtual const char *GetName() const;

    /**
     * Write the player's colour name to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3f30
     * @ghidraAddress PAL: 0x0041c120
     */
    virtual void PrintExtra(std::ostream &stream) const;

    int mPoints = 0;           /*!< The points shown under `NEUTRALIZED!`. +0x08 */
    Player *mPlayer = nullptr; /*!< The player whose track was neutralised. +0x0c */
};

/**
 * Identity that PlayersTrackNeutralizedMsg::Type() reports.
 *
 * This word belongs to PlayersTrackNeutralizedMsg because PlayersTrackNeutralizedMsg::Type() at
 * `0x003e0618` returns it. Several handlers elsewhere read the same word to compare against it,
 * which is the expected shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0324
 * @ghidraAddress PAL: 0x00713abc
 */
extern int g_nPlayersTrackNeutralizedMsgType;
