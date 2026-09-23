#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `8EraseMsg` in the RTTI descriptor at `0x00901cf0`, with Message as its one base. The object is
 * 0x14 bytes and its vtable is at `0x00813160`. The members below are the whole of the class:
 * everything recovered comes from them, and no other routine in the image refers to this type by
 * anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone().
 * InputMap::OnControllerReading(), the one builder, stores the resolved player, the controller
 * reading's position, the track the player's slot 4 reports, and a double-tap flag. The flag is set
 * when the press falls within 400,000,000 of the previous press time the player stores at `+0x40`,
 * and Scratcher widens the erased range to the whole section when it is set.
 *
 * Print() hands `+0x08` to Mid::MBT::Print() and writes the colour name of the player at `+0x04`,
 * which types both. New() initialises the position to kMBTInfinity.
 *
 * The destructor at `0x003db2f0` is compiler-generated and has no declaration here.
 */
class EraseMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    EraseMsg() {
    }

    /**
     * Report an erase press.
     *
     * Inline, with no address of its own. InputMap::OnControllerReading() at `0x00119aa0` expands
     * it on its stack. The four arguments are the four members in declaration order.
     *
     * @param pPlayer The player the controller belongs to.
     * @param position The song position of the reading.
     * @param nTrack The player's track.
     * @param nDoubleTap Non-zero when the press follows the previous one closely.
     */
    EraseMsg(Player *pPlayer, Mid::MBT position, int nTrack, int nDoubleTap)
        : mUnknown04(pPlayer), mUnknown08(position), mUnknown0c(nTrack), mUnknown10(nDoubleTap) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d6b28
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003db3e0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nEraseMsgType.
     * @ghidraAddress 0x003db440
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `EraseMsg`.
     * @ghidraAddress 0x003db450
     */
    virtual const char *Name();

    /**
     * Write the position and the player's colour name, separated by a space, to a diagnostic
     * stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3320
     */
    virtual void Print(std::ostream &stream);

public:
    // Public because Voxer::HandleMessage(), Scratcher::HandleMessage(), and
    // NotePitcher::HandleMessage() reads these directly, through a EraseMsg pointer from outside
    // the hierarchy, and the image exposes no accessor. A friend declaration fits equally well.
    Player *mUnknown04;  // +0x04
    Mid::MBT mUnknown08; // +0x08
    int mUnknown0c;      // +0x0c
    int mUnknown10;      // +0x10
};

/**
 * Identity that EraseMsg::Type() reports.
 *
 * This word belongs to EraseMsg because EraseMsg::Type() at `0x003db440` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0174
 */
extern int g_nEraseMsgType;
