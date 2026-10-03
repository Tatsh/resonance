#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef640`. It has Message as its one base. The object is 0x18 bytes
 * and its vtable is at `0x00812b28`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone() and from the stack build in
 * JamEffectsMgr::PostRemixFxMsg() at `0x001a55d4`. That build stores the vtable `0x007df1e0`
 * rather than `0x00812b28`, a second emission of the same table in the unit that constructs it.
 * Readers of the fields have not been traced, so they are private.
 */
class RemixFXMsg : public Message {
public:
    /**
     * Construct a message with every field unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    RemixFXMsg() {
    }

    /**
     * Report one remix effect toggle.
     *
     * Inline, with no address of its own. JamEffectsMgr::PostRemixFxMsg() expands it on its stack
     * at `0x001a55d4`.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param nEffect The effect type.
     * @param bEnabled Non-zero when the effect is now on for the bar.
     * @param pPlayer The deploying player, JamEffectMsg::mPlayer passed through.
     */
    RemixFXMsg(int nTrack, int nBar, int nEffect, int bEnabled, Player *pPlayer)
        : mTrack(nTrack), mBar(nBar), mEffect(nEffect), mEnabled(bEnabled), mPlayer(pPlayer) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 308.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7070
     * @ghidraAddress PAL: 0x0040ef60
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001a6250
     * @ghidraAddress PAL: 0x001abfb8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRemixFXMsgType.
     * @ghidraAddress NTSC-U/C: 0x001a62b8
     * @ghidraAddress PAL: 0x001ac020
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RemixFXMsg`.
     * @ghidraAddress NTSC-U/C: 0x001a62c8
     * @ghidraAddress PAL: 0x001ac030
     */
    virtual const char *Name();

private:
    int mTrack;      // +0x04
    int mBar;        // +0x08
    int mEffect;     // +0x0c
    int mEnabled;    // +0x10
    Player *mPlayer; // +0x14
};

/**
 * Identity that RemixFXMsg::Type() reports.
 *
 * This word belongs to RemixFXMsg because RemixFXMsg::Type() at `0x001a62b8` returns it, and the
 * registration at `0x003d9818` passes the same value, 308, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d022c
 * @ghidraAddress PAL: 0x007139c4
 */
extern int g_nRemixFXMsgType;
