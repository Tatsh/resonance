#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * Its RTTI descriptor is at `0x00901d50`. It has Message as its one base. The object is 0x8 bytes
 * and its vtable is at `0x00811b20`. The members below are the whole of the class.
 *
 * The payload layout comes from the run of field copies in Clone().
 *
 * The destructor at `0x003e2cc0` is compiler-generated and has no declaration here.
 */
class MetFreqEndedMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory. The payload is left unset.
     *
     * @return The message.
     * @ghidraAddress NTSC-U/C: 0x003d7d80
     * @ghidraAddress PAL: 0x0040fc98
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress NTSC-U/C: 0x003e2db0
     * @ghidraAddress PAL: 0x0041b250
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMetFreqEndedMsgType.
     * @ghidraAddress NTSC-U/C: 0x003e2df8
     * @ghidraAddress PAL: 0x0041b298
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MetFreqEndedMsg`.
     * @ghidraAddress NTSC-U/C: 0x003e2e08
     * @ghidraAddress PAL: 0x0041b2a8
     */
    virtual const char *GetName() const;

    /**
     * Write `MetFreqEndedMsg ` and the word at `+0x04` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x003e44f0
     * @ghidraAddress PAL: 0x0041c720
     */
    virtual void PrintExtra(std::ostream &stream) const;

    /**
     * Whether the finished game world's GrooveWorld::mContinueJukebox was zero.
     *
     * GameManagerImpl::EndGame() sets it at `0x00106d5c` and `0x00106da8`, and MetRenderer's
     * handler at `0x0036bd20` reads it, resuming the jukebox in jukebox mode only while it is zero.
     * Public because both access it directly. +0x04
     */
    int mStopJukebox;
};

/**
 * Identity that MetFreqEndedMsg::Type() reports.
 *
 * This word belongs to MetFreqEndedMsg because MetFreqEndedMsg::Type() at `0x003e2df8` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress NTSC-U/C: 0x006d03f4
 * @ghidraAddress PAL: 0x00713b8c
 */
extern int g_nMetFreqEndedMsgType;
