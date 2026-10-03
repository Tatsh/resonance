#pragma once

#include <iostream>

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x008ef350`. It has CmdMsg as its one base. The object is 0x14 bytes
 * and its vtable is at `0x00811fa0`. The word at `+0x04` belongs to CmdMsg.
 *
 * PrintExtra() writes the colour name of the player at `+0x10`, which types that word. The words at
 * `+0x08` and `+0x0c` are not printed, and no producer of the message has been traced.
 *
 * The destructor at `0x003e12b0` is compiler-generated and has no declaration here.
 */
class BumpMsg : public CmdMsg {
public:
    /**
     * Produce a message with a zero result on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d79e8
     * @ghidraAddress PAL: 0x0040f8e8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e13d0
     * @ghidraAddress PAL: 0x00419828
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nBumpMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e1440
     * @ghidraAddress PAL: 0x00419898
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `BumpMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e1450
     * @ghidraAddress PAL: 0x004198a8
     */
    virtual const char *GetName() const;

    /**
     * Write the player's colour name to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e3fb8
     * @ghidraAddress PAL: 0x0041c1c8
     */
    virtual void PrintExtra(std::ostream &stream) const;

private:
    // Both titles follow the bar and track that BumpPacket includes in the same order.
    int mBar;        // +0x08
    int mTrack;      // +0x0c
    Player *mPlayer; // +0x10
};

/**
 * Identity that BumpMsg::Type() reports.
 *
 * This word belongs to BumpMsg because BumpMsg::Type() at `0x003e1440` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d0374
 * @ghidraAddress PAL: 0x00713b0c
 */
extern int g_nBumpMsgType;
