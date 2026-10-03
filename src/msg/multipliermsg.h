#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008eec78`. It has Message as its one base. The object is 0x10 bytes
 * and its vtable is at `0x007e44e0`. The allocation in New() and the allocation in Clone() report
 * the same size.
 *
 * The payload layout comes from the run of field copies in Clone(), and the stack build in
 * MultiplierPowerup::Deploy() fixes the meaning of the first two words. LocalPlayer's handler reads
 * only mBar, so the other two words are private.
 *
 * The destructor at `0x001caa80` is compiler-generated and has no declaration here.
 */
class MultiplierMsg : public Message {
public:
    /**
     * Construct a message with every payload word indeterminate.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    MultiplierMsg() {
    }

    /**
     * Report a multiplier powerup deployed by a player.
     *
     * Inline, with no address of its own. MultiplierPowerup::Deploy() expands it on its stack at
     * `0x001ca238`, passing 4 as nFactor.
     *
     * @param pPlayer The player who deployed the powerup.
     * @param nBar The bar the multiplier bonus starts at.
     * @param nFactor The multiplier the powerup requests. No reader is recorded, and the title is
     *                inferred from the value the one builder passes.
     */
    MultiplierMsg(Player *pPlayer, int nBar, int nFactor)
        : mPlayer(pPlayer), mBar(nBar), mFactor(nFactor) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 116.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6c60
     * @ghidraAddress PAL: 0x0040eb50
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x001cab30
     * @ghidraAddress PAL: 0x001d09e8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMultiplierMsgType.
     * @ghidraAddress NTSC-U/C: 0x001cab88
     * @ghidraAddress PAL: 0x001d0a40
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MultiplierMsg`.
     * @ghidraAddress NTSC-U/C: 0x001cab98
     * @ghidraAddress PAL: 0x001d0a50
     */
    virtual const char *GetName() const;

private:
    Player *mPlayer; // +0x04, the player who deployed the powerup

public:
    /**
     * The bar the multiplier bonus starts at.
     *
     * Public because LocalPlayer's multiplier handler at `0x0011eb44` reads it directly and ends
     * the bonus eight bars later, and the image has no accessor. +0x08
     */
    int mBar;

private:
    int mFactor; // +0x0c
};

/**
 * Identity that MultiplierMsg::Type() reports.
 *
 * This word belongs to MultiplierMsg because MultiplierMsg::Type() at `0x001cab88` returns it, and
 * the registration at `0x003d9818` passes the same value, 116, as the identity of this class's
 * factory.
 *
 * @ghidraAddress NTSC-U/C: 0x006d019c
 * @ghidraAddress PAL: 0x00713934
 */
extern int g_nMultiplierMsgType;
