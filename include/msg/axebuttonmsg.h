#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12AxeButtonMsg` in the RTTI descriptor at `0x008ef450`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007dd3f0`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The stack builds fix the
 * player at `+0x0c`. Scratcher's build at `0x001d07f4` stores the same scratcher word that its
 * ShowEraseEffectMsg build stores as the player, and Overlay compares that word with
 * HudTrack::mPlayer. AutoRiffer stores 1 at `+0x04` when a riff starts and 0 when it stops. The
 * purpose of the word at `+0x08` is not recovered. Every member is public because
 * AppTunnel::HandleMessage() at `0x004499c4` reads all three directly with no accessor in the
 * image.
 *
 * The destructor at `0x0019a698` is compiler-generated and has no declaration here.
 */
class AxeButtonMsg : public Message {
public:
    /**
     * Construct a message with the payload unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    AxeButtonMsg() {
    }

    /**
     * Report a button change for a player.
     *
     * Inline, with no address of its own. AutoRiffer::OnPitchRiff() at `0x00199278`,
     * AutoRiffer::OnStopRiff() at `0x00199414`, and the Voxer and Scratcher members expand it on
     * their stacks. The three arguments are the three members in declaration order.
     *
     * @param nPressed Non-zero when the button goes down.
     * @param nUnknown08 The word at `+0x08`, 1 from Scratcher and 0 from every other builder.
     * @param pPlayer The player.
     */
    AxeButtonMsg(int nPressed, int nUnknown08, Player *pPlayer)
        : mPressed(nPressed), mUnknown08(nUnknown08), mPlayer(pPlayer) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 412.
     *
     * @return The message.
     * @ghidraAddress 0x003d76b8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0019a748
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAxeButtonMsgType.
     * @ghidraAddress 0x0019a7a0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AxeButtonMsg`.
     * @ghidraAddress 0x0019a7b0
     */
    virtual const char *Name();

    int mPressed;    /*!< Non-zero when the button goes down. +0x04 */
    int mUnknown08;  /*!< Purpose unrecovered. +0x08 */
    Player *mPlayer; /*!< The player. +0x0c */
};

/**
 * Identity that AxeButtonMsg::Type() reports.
 *
 * This word belongs to AxeButtonMsg because AxeButtonMsg::Type() at `0x0019a7a0` returns it, and
 * the registration at `0x003d9818` passes the same value, 412, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d0304
 */
extern int g_nAxeButtonMsgType;
