#pragma once

#include <vector>

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `6WinMsg` in the RTTI descriptor at `0x008f09d0`, with Message as its one base. The object is
 * 0x10 bytes and its vtable is at `0x007ce690`. The allocation in New() and the allocation in
 * Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The vector at `+0x04` comes from the copy constructor at `0x00116eb8`, which allocates one
 * element for every element of the source and moves them with a block copy. Its three words are
 * the start, the finish, and the end of storage.
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

private:
    std::vector<int> mUnknown04; // +0x04
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
