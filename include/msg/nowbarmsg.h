#pragma once

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `9NowBarMsg` in the RTTI descriptor at `0x008efea0`, with Message as its one base. The object is
 * 0x10 bytes and its vtable is at `0x007e5680`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). mPlayer and mLane are public
 * because AppTunnel::HandleMessage() at `0x004496e8` reads them directly with no accessor in the
 * image. It compares mPlayer with the player each tunnel item stores, which types it, and eases
 * the item toward mLane.
 *
 * A second, identical vtable at `0x007de200` is emitted in the guitar and vocal units, and the
 * five writers of mUnknown04 store it.
 *
 * The destructor at `0x0019f970` is compiler-generated and has no declaration here.
 */
class NowBarMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 411.
     *
     * @return The message.
     * @ghidraAddress 0x003d7680
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x0019fa20
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nNowBarMsgType.
     * @ghidraAddress 0x0019fa78
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `NowBarMsg`.
     * @ghidraAddress 0x0019fa88
     */
    virtual const char *Name();

    /**
     * The track of the now bar. +0x04
     *
     * Public because five routines write it directly: AxisControl's OnAxisRegister() at
     * `0x0019ea80` and OnTrackSelect() at `0x0019ec10`, Scratcher's PostNowBarMsg() at `0x001cfd20`
     * and OnTrackSelect() at `0x001d0248`, and Voxer::OnTrackSelect() at `0x001d8840`.
     */
    int mUnknown04;

    Player *mPlayer; /*!< The player the now bar belongs to. +0x08 */
    float mLane;     /*!< The lane AppTunnel eases the item toward. +0x0c */
};

/**
 * Identity that NowBarMsg::Type() reports.
 *
 * This word belongs to NowBarMsg because NowBarMsg::Type() at `0x0019fa78` returns it, and the
 * registration at `0x003d9818` passes the same value, 411, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d02fc
 */
extern int g_nNowBarMsgType;
