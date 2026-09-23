#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12ButtonPowMsg` in the RTTI descriptor at `0x008eec88`, with Message as its one base. The
 * object is 0x10 bytes and its vtable is at `0x008131a8`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). Print() hands `+0x0c` to
 * Mid::MBT::Print() and writes the colour name of the player at `+0x04`, which types both. The
 * purpose of the word at `+0x08` is not recovered. InputMap::OnControllerReading() stores 1 there
 * at both of its builds, once after testing that the play mode is 1 and once as a constant.
 *
 * The destructor at `0x003db130` is compiler-generated and has no declaration here.
 */
class ButtonPowMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    ButtonPowMsg() {
    }

    /**
     * Report a powerup button press.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() expands it on its stack
     * at `0x00119a18` and `0x00119c50`.
     *
     * @param pPlayer The player the controller belongs to.
     * @param nUnknown08 The word at `+0x08`, 1 at both builds.
     * @param position The song position of the reading.
     */
    ButtonPowMsg(Player *pPlayer, int nUnknown08, Mid::MBT position)
        : mPlayer(pPlayer), mUnknown08(nUnknown08), mPosition(position) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d6ae8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003db220
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nButtonPowMsgType.
     * @ghidraAddress 0x003db278
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `ButtonPowMsg`.
     * @ghidraAddress 0x003db288
     */
    virtual const char *Name();

    /**
     * Write the position, the player's colour name, and the word at `+0x08`, separated by
     * spaces, to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003d7df0
     */
    virtual void Print(std::ostream &stream);

    /**
     * The player the controller belongs to.
     *
     * Public because LocalPlayer::HandleMessage() reads it directly at `0x0011f004`, and the image
     * has no accessor. +0x04
     */
    Player *mPlayer;

private:
    int mUnknown08;     // +0x08
    Mid::MBT mPosition; // +0x0c
};

/**
 * Identity that ButtonPowMsg::Type() reports.
 *
 * This word belongs to ButtonPowMsg because ButtonPowMsg::Type() at `0x003db278` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d016c
 */
extern int g_nButtonPowMsgType;
