#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18MultiplierStateMsg` in the RTTI descriptor at `0x008ef100`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007cffa8`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The producer at `0x0011eb20`
 * writes the player, its base multiplier plus one, and its bonus multiplier.
 *
 * Every member is public because Overlay::OnMultiplierState() at `0x00420408` reads it directly
 * with no accessor in the image. It compares mPlayer with HudTrack::mPlayer, shows the sum of
 * mMultiplier and mBonus, and selects the hot material when mBonus is non-zero.
 *
 * The destructor at `0x00122560` is compiler-generated and has no declaration here.
 */
class MultiplierStateMsg : public Message {
public:
    /**
     * Construct a message with every payload word indeterminate.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    MultiplierStateMsg() {
    }

    /**
     * Report a player's multiplier.
     *
     * Inline, with no address of its own. LocalPlayer expands it on its stack at `0x0011eb98` and
     * `0x0011ec90`.
     *
     * @param pPlayer The player whose multiplier changed.
     * @param nMultiplier The base multiplier.
     * @param nBonus The multiplier added on top of the base, or zero.
     */
    MultiplierStateMsg(Player *pPlayer, int nMultiplier, int nBonus)
        : mPlayer(pPlayer), mMultiplier(nMultiplier), mBonus(nBonus) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 117.
     *
     * @return The message.
     * @ghidraAddress 0x003d6c98
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00122610
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMultiplierStateMsgType.
     * @ghidraAddress 0x00122668
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MultiplierStateMsg`.
     * @ghidraAddress 0x00122678
     */
    virtual const char *Name();

    Player *mPlayer; /*!< The player whose multiplier changed. +0x04 */
    int mMultiplier; /*!< The base multiplier. +0x08 */
    int mBonus;      /*!< The multiplier added on top of the base, or zero. +0x0c */
};

/**
 * Identity that MultiplierStateMsg::Type() reports.
 *
 * This word belongs to MultiplierStateMsg because MultiplierStateMsg::Type() at `0x00122668`
 * returns it, and the registration at `0x003d9818` passes the same value, 117, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d01a4
 */
extern int g_nMultiplierStateMsgType;
