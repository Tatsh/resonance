#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `14FreestyleFXMsg` in the RTTI descriptor at `0x00902260`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007ce648`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The names come from Gamer's
 * two builds, which store a track, a first bar, and an end bar (the first bar plus 8 at
 * `0x001111b8`). Readers of the fields have not been traced, so they are private by default.
 *
 * The destructor at `0x00116450` is compiler-generated and has no declaration here. The routine
 * at `0x00116488` is a further emission of the type-information accessor.
 */
class FreestyleFXMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    FreestyleFXMsg() {
    }

    /**
     * Report a freestyle effect over a range of bars on a track.
     *
     * Inline, with no address of its own. Gamer's builds at `0x001111ac` and `0x00111f20` expand
     * it on their stacks. The three arguments are the three members in declaration order.
     *
     * @param nTrack The track.
     * @param nBar The first bar.
     * @param nEndBar The bar the effect ends at.
     */
    FreestyleFXMsg(int nTrack, int nBar, int nEndBar)
        : mTrack(nTrack), mBar(nBar), mEndBar(nEndBar) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 322.
     *
     * @return The message.
     * @ghidraAddress 0x003d7398
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00116500
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nFreestyleFXMsgType.
     * @ghidraAddress 0x00116558
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `FreestyleFXMsg`.
     * @ghidraAddress 0x00116568
     */
    virtual const char *Name();

private:
    int mTrack;  // +0x04
    int mBar;    // +0x08
    int mEndBar; // +0x0c
};

/**
 * Identity that FreestyleFXMsg::Type() reports.
 *
 * This word belongs to FreestyleFXMsg because FreestyleFXMsg::Type() at `0x00116558` returns it,
 * and the registration at `0x003d9818` passes the same value, 322, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d029c
 */
extern int g_nFreestyleFXMsgType;
