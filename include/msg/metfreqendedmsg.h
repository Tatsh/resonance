#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `15MetFreqEndedMsg` in the RTTI descriptor at `0x00901d50`, with Message as its one base. The
 * object is 0x8 bytes and its vtable is at `0x00811b20`. The members below are the whole of the
 * class: everything recovered comes from them.
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
     * @ghidraAddress 0x003d7d80
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x003e2db0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nMetFreqEndedMsgType.
     * @ghidraAddress 0x003e2df8
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `MetFreqEndedMsg`.
     * @ghidraAddress 0x003e2e08
     */
    virtual const char *Name();

    /**
     * Write `MetFreqEndedMsg ` and the word at `+0x04` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x003e44f0
     */
    virtual void Print(std::ostream &stream);

    /**
     * Whether the finished game world's GrooveWorld::mUnknownb8 was zero.
     *
     * GameManagerImpl::EndGame() sets it at `0x00106d5c` and `0x00106da8`, and MetRenderer's
     * handler at `0x0036bd20` reads it. Public because both access it directly. +0x04
     */
    int mUnknownb8Clear;
};

/**
 * Identity that MetFreqEndedMsg::Type() reports.
 *
 * This word belongs to MetFreqEndedMsg because MetFreqEndedMsg::Type() at `0x003e2df8` returns
 * it. Several handlers elsewhere read the same word to compare against it, which is the expected
 * shape for a registered identity and does not make the word theirs.
 *
 * @ghidraAddress 0x006d03f4
 */
extern int g_nMetFreqEndedMsgType;
