#pragma once

#include <iostream>

#include "msg/cmdmsg.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `7BumpMsg` in the RTTI descriptor at `0x008ef350`, with CmdMsg as its one base. The object is
 * 0x14 bytes and its vtable is at `0x00811fa0`. The word at `+0x04` belongs to CmdMsg.
 *
 * Print() writes the colour name of the player at `+0x10`, which types that word. The words at
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
     * @ghidraAddress 0x003d79e8
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e13d0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nBumpMsgType.
     * @ghidraAddress 0x003e1440
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `BumpMsg`.
     * @ghidraAddress 0x003e1450
     */
    virtual const char *Name();

    /**
     * Write the player's colour name to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e3fb8
     */
    virtual void Print(std::ostream &stream);

private:
    int mUnknown08;  // +0x08
    int mUnknown0c;  // +0x0c
    Player *mPlayer; // +0x10
};

/**
 * Identity that BumpMsg::Type() reports.
 *
 * This word belongs to BumpMsg because BumpMsg::Type() at `0x003e1440` returns it. Several
 * handlers elsewhere read the same word to compare against it, which is the expected shape for a
 * registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0374
 */
extern int g_nBumpMsgType;
