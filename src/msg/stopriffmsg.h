#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008efff0`. It has Message as its one base. The object is 0x14 bytes
 * and its vtable is at `0x00813358`. The members below are the whole of the class. No other
 * routine in the image refers to this type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(). PrintExtra() hands `+0x0c` to
 * Mid::MBT::Print(), writes the colour name of the player at `+0x08`, and labels `+0x04` as `b#`.
 * The word at `+0x10` is not printed.
 *
 * Every member is public because the handlers AutoRiffer dispatches to at `0x001992e0` and Voxer's
 * slot 3 at `0x001d90a4` read the payload with no accessor in the image, and the two stack builds
 * at `0x00119e74` and `0x0011da7c` write all four words.
 *
 * The destructor at `0x003da708` is compiler-generated and has no declaration here.
 */
class StopRiffMsg : public Message {
public:
    /**
     * Identity that Type() reports.
     *
     * @ghidraAddress NTSC-U/C: 0x006d013c
     * @ghidraAddress PAL: 0x007138d4
     */
    static int sID;

    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    StopRiffMsg() {
    }

    /**
     * Report the end of a riff.
     *
     * Inline, with no address of its own. The routine at `0x0011da68`, which
     * InputMap::OnControllerReading() calls, and the build at `0x00119e74` expand it on their
     * stacks. The four arguments are the four members in declaration order.
     *
     * @param nButton The button, labelled `b#` by PrintExtra().
     * @param pPlayer The player whose riff stops.
     * @param position The song position of the stop.
     * @param nTrack The player's track.
     */
    StopRiffMsg(int nButton, Player *pPlayer, Mid::MBT position, int nTrack)
        : mButton(nButton), mPlayer(pPlayer), mPosition(position), mTrack(nTrack) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d6968
     * @ghidraAddress PAL: 0x0040e858
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003da7f8
     * @ghidraAddress PAL: 0x00412c30
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return sID.
     * @ghidraAddress NTSC-U/C: 0x003da858
     * @ghidraAddress PAL: 0x00412c90
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `StopRiffMsg`.
     * @ghidraAddress NTSC-U/C: 0x003da868
     * @ghidraAddress PAL: 0x00412ca0
     */
    virtual const char *GetName() const;

    /**
     * Write the position, a space, the player's colour name, ` b#`, and the word at `+0x04` to a
     * diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3098
     * @ghidraAddress PAL: 0x0041b558
     */
    virtual void PrintExtra(std::ostream &stream) const;

    int mButton;        /*!< The button, labelled `b#` by PrintExtra(). +0x04 */
    Player *mPlayer;    /*!< The player whose riff stops. +0x08 */
    Mid::MBT mPosition; /*!< The song position of the stop. +0x0c */
    int mTrack;         /*!< The player's track, from its slot 4. +0x10 */
};
