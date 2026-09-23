#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `12PitchRiffMsg` in the RTTI descriptor at `0x008eed18`, with Message as its one base. The
 * object is 0x14 bytes and its vtable is at `0x008133a0`. The members below are the whole of the
 * class: everything recovered comes from them, and no other routine in the image refers to this
 * type by anything but its vtable.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * Print() hands `+0x0c` to Mid::MBT::Print(), writes the colour name of the player at `+0x08`,
 * and labels `+0x04` as `b#`. New() initialises the position to kMBTInfinity.
 *
 * The destructor at `0x003da530` is compiler-generated and has no declaration here.
 */
class PitchRiffMsg : public Message {
public:
    /**
     * Construct a message with the position at kMBTInfinity and the rest unset.
     *
     * Inline. New() expands it. A declaration is required because the class declares a second
     * constructor.
     */
    PitchRiffMsg() {
    }

    /**
     * Report the start of a riff.
     *
     * Inline, with no address of its own. The routine at `0x0011d9b0`, which
     * InputMap::OnControllerReading() calls, expands it on its stack at `0x0011d9fc`. The four
     * arguments are the four members in declaration order. The last is the track the player's
     * slot 4 reports.
     *
     * @param nUnknown04 The word Print() labels `b#`.
     * @param pPlayer The player whose riff starts.
     * @param position The song position of the start.
     * @param nTrack The player's track.
     */
    PitchRiffMsg(int nUnknown04, Player *pPlayer, Mid::MBT position, int nTrack)
        : mUnknown04(nUnknown04), mUnknown08(pPlayer), mUnknown0c(position), mUnknown10(nTrack) {
    }

    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d6928
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003da620
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nPitchRiffMsgType.
     * @ghidraAddress 0x003da680
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `PitchRiffMsg`.
     * @ghidraAddress 0x003da690
     */
    virtual const char *Name();

    /**
     * Write the position, a space, the player's colour name, ` b#`, and the word at `+0x04` to a
     * diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e2fd0
     */
    virtual void Print(std::ostream &stream);

public:
    // Public because Scratcher::HandleMessage() reads these directly, through a PitchRiffMsg
    // pointer from outside the hierarchy, and the image exposes no accessor. A friend declaration
    // fits equally well.
    int mUnknown04;      // +0x04
    Player *mUnknown08;  // +0x08
    Mid::MBT mUnknown0c; // +0x0c
    int mUnknown10;      // +0x10
};

/**
 * Identity that PitchRiffMsg::Type() reports.
 *
 * This word belongs to PitchRiffMsg because PitchRiffMsg::Type() at `0x003da680` returns it.
 * Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0134
 */
extern int g_nPitchRiffMsgType;
