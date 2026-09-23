#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `10RemixFXMsg` in the RTTI descriptor at `0x008ef640`, with Message as its one base. The object
 * is 0x18 bytes and its vtable is at `0x00812b28`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
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
     * @param nUnknown10 JamEffectMsg::mUnknown10, passed through.
     */
    RemixFXMsg(int nTrack, int nBar, int nEffect, int bEnabled, int nUnknown10)
        : mTrack(nTrack), mBar(nBar), mEffect(nEffect), mEnabled(bEnabled), mUnknown14(nUnknown10) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 308.
     *
     * @return The message.
     * @ghidraAddress 0x003d7070
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001a6250
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRemixFXMsgType.
     * @ghidraAddress 0x001a62b8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RemixFXMsg`.
     * @ghidraAddress 0x001a62c8
     */
    virtual const char *Name();

private:
    int mTrack;     // +0x04
    int mBar;       // +0x08
    int mEffect;    // +0x0c
    int mEnabled;   // +0x10
    int mUnknown14; // +0x14, JamEffectMsg::mUnknown10 passed through
};

/**
 * Identity that RemixFXMsg::Type() reports.
 *
 * This word belongs to RemixFXMsg because RemixFXMsg::Type() at `0x001a62b8` returns it, and the
 * registration at `0x003d9818` passes the same value, 308, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d022c
 */
extern int g_nRemixFXMsgType;
