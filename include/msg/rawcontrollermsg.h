#pragma once

#include <iostream>

#include "mid/mbt.h"
#include "msg/message.h"
#include "msg/metcontrollerreading.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `16RawControllerMsg` in the RTTI descriptor at `0x008efce0`, with Message as its one base. The
 * object is 0x18 bytes and its vtable is at `0x00813478`.
 *
 * The payload is one controller reading at `+0x04` and a song position at `+0x14`. Clone() copies
 * the reading as two eight-byte pairs and the position as one word, Print() hands the reading to
 * MetControllerReading::Print(), and New() initialises only the position, to kMBTInfinity.
 * MetaGameWorld builds the message on its stack from the four arguments of its RawController
 * slot, and MetRenderer reads the reading in place. Both are outside the hierarchy, and the image
 * exposes no accessor, so both members are public.
 *
 * The destructor at `0x003da110` is compiler-generated and has no declaration here.
 */
class RawControllerMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. Only the position is
     * initialised.
     *
     * @return The message.
     * @ghidraAddress 0x003d6868
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003da200
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nRawControllerMsgType.
     * @ghidraAddress 0x003da268
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `RawControllerMsg`.
     * @ghidraAddress 0x003da278
     */
    virtual const char *Name();

    /**
     * Write the reading to a diagnostic stream through MetControllerReading::Print().
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e2fb0
     */
    virtual void Print(std::ostream &stream);

    MetControllerReading mReading; /*!< The reading. +0x04 */
    Mid::MBT mPosition;            /*!< Song position, kMBTInfinity until set. +0x14 */
};

/**
 * Identity that RawControllerMsg::Type() reports.
 *
 * This word belongs to RawControllerMsg because RawControllerMsg::Type() at `0x003da268` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d0118
 */
extern int g_nRawControllerMsgType;
