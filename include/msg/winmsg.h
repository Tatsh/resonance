#pragma once

#include <vector>

#include "msg/message.h"

class Player;

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `6WinMsg` in the RTTI descriptor at `0x008f09d0`, with Message as its one base. The object is
 * 0x10 bytes and its vtable is at `0x007ce690`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(). The vector at `+0x04` comes
 * from the copy constructor at `0x00116eb8`, which allocates one element for every element of the
 * source and moves them with a block copy. Its three words are the start, the finish, and the end
 * of storage.
 *
 * The vector is public because two handlers outside the class read it directly with no accessor in
 * the image. Overlay::OnWin() at `0x0041e020` searches it for HudTrack::mPlayer with std::find at
 * `0x0041e1d0`. That search types the elements as players. TnlArena::HandleMessage() tests the
 * vector for emptiness.
 *
 * The destructors at `0x00116238` and `0x003e0c00` are compiler-generated, release mWinners, and
 * have no declaration here. The routine at `0x001162f0` is a further emission of the
 * type-information accessor.
 */
class WinMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 421.
     *
     * @return The message.
     * @ghidraAddress 0x003d78c0
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x00116368
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nWinMsgType.
     * @ghidraAddress 0x001163e0
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `WinMsg`.
     * @ghidraAddress 0x001163f0
     */
    virtual const char *Name();

    std::vector<Player *> mWinners; /*!< The winning players. +0x04 */
};

/**
 * Identity that WinMsg::Type() reports.
 *
 * This word belongs to WinMsg because WinMsg::Type() at `0x001163e0` returns it, and the
 * registration at `0x003d9818` passes the same value, 421, as the identity of this class's
 * factory.
 *
 * @ghidraAddress 0x006d034c
 */
extern int g_nWinMsgType;
