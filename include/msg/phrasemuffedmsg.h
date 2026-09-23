#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `15PhraseMuffedMsg` in the RTTI descriptor at `0x00902280`, with Message as its one base. The
 * object is 0x14 bytes and its vtable is at `0x00812348`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() labels `+0x04` as a
 * track number and `+0x10` as `tried`, dispatches Player::Print() through `+0x08`, and hands
 * `+0x0c` to Mid::MBT::Print(). New() initialises that position to kMBTInfinity. mPlayer is public
 * because Overlay::OnPhraseMuffed() at `0x0042b178` reads it directly with no accessor in the
 * image, comparing it with HudTrack::mPlayer. The other three are public because AppTunnel's
 * muffed-phrase handler at `0x00447ba8` reads them directly.
 *
 * The destructor at `0x003dff48` is compiler-generated and has no declaration here.
 */
class PhraseMuffedMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    PhraseMuffedMsg() {
    }

    /**
     * Report a muffed phrase.
     *
     * Inline, with no address of its own. Catcher::PostPhraseMuffedMsg() at `0x001ad470` expands
     * it on its stack with the catcher's track and player. The builder stores the position
     * straight from its argument register with no IsFiniteMBT() call, so the position arrives
     * already a Mid::MBT. The four arguments are the four members in declaration order.
     *
     * @param nTrack The track.
     * @param pPlayer The player who muffed the phrase.
     * @param position The song position of the miss.
     * @param nTried Non-zero when the player had attempted the phrase.
     */
    PhraseMuffedMsg(int nTrack, Player *pPlayer, Mid::MBT position, int nTried)
        : mTrack(nTrack), mPlayer(pPlayer), mPosition(position), mTried(nTried) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d76f0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e0038
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPhraseMuffedMsgType.
     * @ghidraAddress 0x003e0098
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PhraseMuffedMsg`.
     * @ghidraAddress 0x003e00a8
     */
    virtual const char *Name();

    /**
     * Write `tr#`, the track, a space, the player, a space, the position, ` tried:`, and the word
     * at `+0x10` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e4350
     */
    virtual void Print(std::ostream &stream);

    int mTrack;         /*!< The track. +0x04 */
    Player *mPlayer;    /*!< The player who muffed the phrase. +0x08 */
    Mid::MBT mPosition; /*!< The song position of the miss. +0x0c */
    int mTried;         /*!< Non-zero when the player had attempted the phrase. +0x10 */
};

/**
 * Identity that PhraseMuffedMsg::Type() reports.
 *
 * This word belongs to PhraseMuffedMsg because PhraseMuffedMsg::Type() at `0x003e0098` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d030c
 */
extern int g_nPhraseMuffedMsgType;
