#pragma once

#include <iostream>

#include "mid/tick.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901b70`. It has Message as its one base. The object is 0x18 bytes
 * and its vtable is at `0x00812588`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). PrintExtra() hands `+0x04` to
 * Sch::Tick::Print() and writes the colour name of the player at `+0x10`. The track at `+0x08` and
 * the gem at `+0x0c` are printed without labels, and the word at `+0x14` is not printed. The two
 * names come from Catcher::SimulateRemoteGem() at `0x001ace78`, which stores the catcher's track
 * and the gem TrackData::FindGemAtOrAfter() reports.
 *
 * Every member is public because AppTunnel's gem handler at `0x00447638` reads all five directly
 * with no accessor in the image. A non-zero mGhost selects the ghost gem there. Every stack build
 * clears it except PhraseMgr::AddGem()'s at `0x001bae48`, which stores its own flag.
 *
 * The destructor at `0x003df450` is compiler-generated and has no declaration here.
 */
class GemMsg : public Message {
public:
    /**
     * Construct a message with only the position set, to kTickInfinity.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    GemMsg() {
    }

    /**
     * Report a gem played on a track.
     *
     * Inline, with no address of its own. Every stack build in the image expands it. Most clear
     * mGhost (Catcher::SimulateRemoteGem() at `0x001ad048`, Catcher::CatchGem() at `0x001ac25c`,
     * PhraseMgr::PostGemMsg() at `0x001ba86c`, and PhraseMgr::AddGem() at `0x001baed0`), and
     * PhraseMgr::AddGem()'s build at `0x001bae48` passes a flag of its own.
     *
     * @param position The song position of the gem.
     * @param nTrack The track the gem lies on.
     * @param nGem The gem.
     * @param pPlayer The player who played it.
     * @param nGhost Non-zero for a ghost gem.
     */
    GemMsg(Sch::Tick position, int nTrack, int nGem, Player *pPlayer, int nGhost = 0)
        : mPosition(position), mTrack(nTrack), mGem(nGem), mPlayer(pPlayer), mGhost(nGhost) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d74f8
     * @ghidraAddress PAL: 0x0040f3f8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003df540
     * @ghidraAddress PAL: 0x00417998
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGemMsgType.
     * @ghidraAddress NTSC-U/C: 0x003df5a8
     * @ghidraAddress PAL: 0x00417a00
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GemMsg`.
     * @ghidraAddress NTSC-U/C: 0x003df5b8
     * @ghidraAddress PAL: 0x00417a10
     */
    virtual const char *GetName() const;

    /**
     * Write the track, the position, the gem, and the player's colour name, separated by spaces,
     * to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003d8830
     * @ghidraAddress PAL: 0x00410c48
     */
    virtual void PrintExtra(std::ostream &stream) const;

    Sch::Tick mPosition; /*!< The song position of the gem. +0x04 */
    int mTrack;          /*!< The track the gem lies on. +0x08 */
    int mGem;            /*!< The gem, which AppTunnel uses as the lane. +0x0c */
    Player *mPlayer;     /*!< The player who played it. +0x10 */
    int mGhost;          /*!< Non-zero for a ghost gem. +0x14 */
};

/**
 * Identity that GemMsg::Type() reports.
 *
 * This word belongs to GemMsg because GemMsg::Type() at `0x003df5a8` returns it. Several handlers
 * elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d02cc
 * @ghidraAddress PAL: 0x00713a64
 */
extern int g_nGemMsgType;
