#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18ShowEraseEffectMsg` in the RTTI descriptor at `0x00901df0`, with Message as its one base. The
 * object is 0x18 bytes and its vtable is at `0x007ddb50`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). Scratcher::EraseGemRange()
 * builds it on the stack at `0x001d0194` with the erased range as a first bar and an end bar one
 * past the last. A single erased bar therefore gives an end one greater than its start.
 *
 * mPlayer, mFirstBar, and mEndBar are public because Overlay::OnShowEraseEffect() at `0x0041fba0`
 * reads them directly with no accessor in the image. It compares mPlayer with HudTrack::mPlayer
 * and shows `BAR ERASED` when `mEndBar - mFirstBar < 2`, `TRACK ERASED` otherwise. mTrack is
 * public because AppTunnel's erase handler at `0x004481d0` reads it directly. Scratcher stores its
 * own track there. The purpose of the word at `+0x14` is not recovered.
 */
class ShowEraseEffectMsg : public Message {
public:
    /**
     * Construct a message with every field unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    ShowEraseEffectMsg() {
    }

    /**
     * Report one erased range.
     *
     * Inline, with no address of its own. AxePhraseMaker::Erase() expands it on its stack at
     * `0x0019c074` with 1 in the word at `+0x14`.
     *
     * @param pPlayer The player who erased the range.
     * @param nTrack The erased track.
     * @param nFirstBar The first erased bar.
     * @param nEndBar The bar one past the last erased bar.
     * @param nUnknown14 Stored in the word at `+0x14`, whose purpose is not recovered.
     */
    ShowEraseEffectMsg(Player *pPlayer, int nTrack, int nFirstBar, int nEndBar, int nUnknown14)
        : mPlayer(pPlayer), mTrack(nTrack), mFirstBar(nFirstBar), mEndBar(nEndBar),
          mUnknown14(nUnknown14) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 315.
     *
     * @return The message.
     * @ghidraAddress 0x003d7210
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0019d6b0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nShowEraseEffectMsgType.
     * @ghidraAddress 0x0019d718
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ShowEraseEffectMsg`.
     * @ghidraAddress 0x0019d728
     */
    virtual const char *Name();

    Player *mPlayer; /*!< The player who erased the range. +0x04 */
    int mTrack;      /*!< The erased track. +0x08 */
    int mFirstBar;   /*!< The first erased bar. +0x0c */
    int mEndBar;     /*!< The bar one past the last erased bar. +0x10 */

private:
    int mUnknown14; // +0x14
};

/**
 * Identity that ShowEraseEffectMsg::Type() reports.
 *
 * This word belongs to ShowEraseEffectMsg because ShowEraseEffectMsg::Type() at `0x0019d718`
 * returns it, and the registration at `0x003d9818` passes the same value, 315, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d0264
 */
extern int g_nShowEraseEffectMsgType;
