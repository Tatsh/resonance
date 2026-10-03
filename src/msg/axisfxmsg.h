#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901b00`. It has Message as its one base. The object is 0x14 bytes
 * and its vtable is at `0x00813280`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). PrintExtra() hands `+0x0c` to
 * Mid::MBT::Print() and writes the colour name of the player at `+0x04`, which types both. The
 * float at `+0x08` is printed without a label, and the word at `+0x10` is not printed.
 * InputMap::OnControllerReading(), the one builder, stores the reading's axis value at `+0x08` and
 * the track the player's slot 4 reports at `+0x10`.
 *
 * The destructor at `0x003dabd8` is compiler-generated and has no declaration here.
 */
class AxisFXMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    AxisFXMsg() {
    }

    /**
     * Report an effect-axis movement.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x001199ac` expands
     * it on its stack.
     *
     * @param pPlayer The player the controller belongs to.
     * @param flValue The axis value.
     * @param position The song position of the reading.
     * @param nTrack The player's track.
     */
    AxisFXMsg(Player *pPlayer, float flValue, Mid::MBT position, int nTrack)
        : mPlayer(pPlayer), mValue(flValue), mPosition(position), mTrack(nTrack) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6a28
     * @ghidraAddress PAL: 0x0040e918
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003dacc8
     * @ghidraAddress PAL: 0x00413100
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nAxisFXMsgType.
     * @ghidraAddress NTSC-U/C: 0x003dad28
     * @ghidraAddress PAL: 0x00413160
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `AxisFXMsg`.
     * @ghidraAddress NTSC-U/C: 0x003dad38
     * @ghidraAddress PAL: 0x00413170
     */
    virtual const char *GetName() const;

    /**
     * Write the position, the player's colour name, and the float at `+0x08`, separated by
     * spaces, to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3240
     * @ghidraAddress PAL: 0x0040fe08
     */
    virtual void PrintExtra(std::ostream &stream) const;

private:
    Player *mPlayer; // +0x04

public:
    /**
     * The axis value. +0x08
     *
     * Public because AxeFX::DispatchPriv() at `0x0019b538` and AxeFX::OnAxisFX() at `0x0019b498`
     * read it directly, and the image has no accessor.
     */
    float mValue;

private:
    Mid::MBT mPosition; // +0x0c
    int mTrack;         // +0x10
};

/**
 * Identity that AxisFXMsg::Type() reports.
 *
 * This word belongs to AxisFXMsg because AxisFXMsg::Type() at `0x003dad28` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0154
 * @ghidraAddress PAL: 0x007138ec
 */
extern int g_nAxisFXMsgType;
