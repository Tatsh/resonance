#pragma once

#include "msg/cmdmsg.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `18EnableFreestyleMsg` in the RTTI descriptor at `0x008f09e0`, with CmdMsg as its one base. The
 * object is 0x10 bytes and its vtable is at `0x007e4570`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The word at `+0x04` belongs to CmdMsg, which New() zeroes. The two words after it belong to this
 * class, for the reason CmdMsg records.
 */
class EnableFreestyleMsg : public CmdMsg {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 414.
     *
     * @return The message.
     * @ghidraAddress 0x003d7730
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001ca8c8
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nEnableFreestyleMsgType.
     * @ghidraAddress 0x001ca930
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `EnableFreestyleMsg`.
     * @ghidraAddress 0x001ca940
     */
    virtual const char *Name();

private:
    int mUnknown08; // +0x08
    int mUnknown0c; // +0x0c
};

/**
 * Identity that EnableFreestyleMsg::Type() reports.
 *
 * This word belongs to EnableFreestyleMsg because EnableFreestyleMsg::Type() at `0x001ca930`
 * returns it, and the registration at `0x003d9818` passes the same value, 414, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d0314
 */
extern int g_nEnableFreestyleMsgType;
