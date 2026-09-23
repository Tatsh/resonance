#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `8CatchMsg` in the RTTI descriptor at `0x00901e00`, with Message as its one base. The object is
 * 0x20 bytes and its vtable is at `0x007e0a28`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Every member is public because
 * code outside the class reads it directly with no accessor in the image. AppTunnel's catch
 * handler at `0x00447938` reads mGem and mPlayer, and Overlay::OnCatch() at `0x0041fed8` reads the
 * rest. Overlay compares mPlayer with HudTrack::mPlayer, draws the catch progress from mCaught over
 * mTotal, converts mTick to a bar by the 1920 ticks of a bar before looking mTrack up, and resets
 * its miss count when mHit is set.
 *
 * The destructor at `0x001b0fb8` is compiler-generated and has no declaration here.
 */
class CatchMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    CatchMsg() {
    }

    /**
     * Report a caught or missed gem.
     *
     * Inline, with no address of its own. Catcher expands it on its stack for a miss in Slot7() at
     * `0x001abf08` and PostCatchMsg() at `0x001ac470`, and for a catch in Slot8() at `0x001ac208`
     * and SimulateRemoteGem() at `0x001ad004`. The seven arguments are the seven members in
     * declaration order.
     *
     * @param nTick The scheduler time of the gem.
     * @param nTrack The track the gem lies on.
     * @param nGem The gem.
     * @param nHit Non-zero for a caught gem, zero for a miss.
     * @param pPlayer The catching player.
     * @param nCaught The gems caught so far in the phrase, or zero.
     * @param nTotal The gems the phrase requires, or zero.
     */
    CatchMsg(int nTick, int nTrack, int nGem, int nHit, Player *pPlayer, int nCaught, int nTotal)
        : mTick(nTick), mTrack(nTrack), mGem(nGem), mHit(nHit), mPlayer(pPlayer), mCaught(nCaught),
          mTotal(nTotal) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 408.
     *
     * @return The message.
     * @ghidraAddress 0x003d75c0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001b1068
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nCatchMsgType.
     * @ghidraAddress 0x001b10e0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `CatchMsg`.
     * @ghidraAddress 0x001b10f0
     */
    virtual const char *Name();

    int mTick;       /*!< The scheduler time of the gem. +0x04 */
    int mTrack;      /*!< The track the gem lies on. +0x08 */
    int mGem;        /*!< The gem, which AppTunnel uses as the lane. +0x0c */
    int mHit;        /*!< Non-zero for a caught gem, zero for a miss. +0x10 */
    Player *mPlayer; /*!< The catching player. +0x14 */
    int mCaught;     /*!< The gems caught so far in the phrase. +0x18 */
    int mTotal;      /*!< The gems the phrase requires. +0x1c */
};

/**
 * Identity that CatchMsg::Type() reports.
 *
 * This word belongs to CatchMsg because CatchMsg::Type() at `0x001b10e0` returns it, and the
 * registration at `0x003d9818` passes the same value, 408, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02e4
 */
extern int g_nCatchMsgType;
