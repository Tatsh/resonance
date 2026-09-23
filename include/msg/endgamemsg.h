#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `10EndGameMsg` in the RTTI descriptor at `0x008ef3c0`, with Message as its one base. The object
 * is 0x8 bytes and its vtable is at `0x007dc598`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x00193908` is compiler-generated and has no declaration here.
 */
class EndGameMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 433.
     *
     * @return The message.
     * @ghidraAddress 0x003d7b88
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001939b8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nEndGameMsgType.
     * @ghidraAddress 0x00193a00
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `EndGameMsg`.
     * @ghidraAddress 0x00193a10
     */
    virtual const char *Name();

    /**
     * Non-zero to start another local game at once rather than return to the front end.
     *
     * Public because GameManagerImpl::OnEndGame() at `0x0010c150` reads it directly and passes it
     * to EndGame(), which queues a BeginGameLocalMsg when it is set. The image has no accessor.
     * +0x04
     */
    int mRestart;
};

/**
 * Identity that EndGameMsg::Type() reports.
 *
 * This word belongs to EndGameMsg because EndGameMsg::Type() at `0x00193a00` returns it, and the
 * registration at `0x003d9818` passes the same value, 433, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d03ac
 */
extern int g_nEndGameMsgType;
