#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `6GemMsg` in the RTTI descriptor at `0x00901b70`, with Message as its one base. The object is
 * 0x18 bytes and its vtable is at `0x00812588`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() hands `+0x04` to
 * Mid::MBT::Print() and writes the colour name of the player at `+0x10`. The track at `+0x08` and
 * the gem at `+0x0c` are printed without labels, and the word at `+0x14` is not printed. The two
 * names come from Catcher::SimulateRemoteGem() at `0x001ace78`, which stores the catcher's track
 * and the gem TrackData::FindGemAtOrAfter() reports.
 *
 * The destructor at `0x003df450` is compiler-generated and has no declaration here.
 */
class GemMsg : public Message {
public:
    /**
     * Construct a message with only the position set, to kMBTInfinity.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    GemMsg() {
    }

    /**
     * Report a gem played on a track.
     *
     * Inline, with no address of its own. Every stack build in the image expands it and clears the
     * word at `+0x14` (Catcher::SimulateRemoteGem() at `0x001ad048`, Catcher::Slot8() at
     * `0x001ac25c`, and PhraseMgr::PostGemMsg() at `0x001ba86c`, among others).
     *
     * @param position The song position of the gem.
     * @param nTrack The track the gem lies on.
     * @param nGem The gem.
     * @param pPlayer The player who played it.
     */
    GemMsg(Mid::MBT position, int nTrack, int nGem, Player *pPlayer)
        : mPosition(position), mTrack(nTrack), mGem(nGem), mPlayer(pPlayer), mUnknown14(0) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d74f8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003df540
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nGemMsgType.
     * @ghidraAddress 0x003df5a8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `GemMsg`.
     * @ghidraAddress 0x003df5b8
     */
    virtual const char *Name();

    /**
     * Write the track, the position, the gem, and the player's colour name, separated by spaces,
     * to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003d8830
     */
    virtual void Print(std::ostream &stream);

private:
    Mid::MBT mPosition; // +0x04
    int mTrack;         // +0x08
    int mGem;           // +0x0c
    Player *mPlayer;    // +0x10
    int mUnknown14;     // +0x14
};

/**
 * Identity that GemMsg::Type() reports.
 *
 * This word belongs to GemMsg because GemMsg::Type() at `0x003df5a8` returns it. Several handlers
 * elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d02cc
 */
extern int g_nGemMsgType;
