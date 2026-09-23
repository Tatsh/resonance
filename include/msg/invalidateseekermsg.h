#pragma once

#include "msg/message.h"

/**
 * Event the game passes between a MsgSource and a MsgSink.
 *
 * `19InvalidateSeekerMsg` in the RTTI descriptor at `0x008ef3d0`, with Message as its one base.
 * The object is 0xc bytes and its vtable is at `0x007ce720`. The allocation in New() and the
 * allocation in Clone() report the same size, which measures the class twice.
 *
 * The payload layout comes from the run of field copies in Clone(), so the offsets and widths are
 * recovered but the purpose of each field is not. Readers of the fields have not been traced, so
 * they are private by default.
 *
 * The destructor at `0x00116000` is compiler-generated and has no declaration here. The routine
 * at `0x00116038` is a further emission of the type-information accessor.
 */
class InvalidateSeekerMsg : public Message {
public:
    /**
     * Produce a default-constructed message on the heap.
     *
     * The translation unit at `0x003d9818` registers this factory against identity 317.
     *
     * @return The message.
     * @ghidraAddress 0x003d7280
     */
    static Message *New();

    /**
     * Produce a heap copy of this message.
     *
     * @return The copy.
     * @ghidraAddress 0x001160b0
     */
    virtual Message *Clone();

    /**
     * Report this message's registered identity.
     *
     * @return g_nInvalidateSeekerMsgType.
     * @ghidraAddress 0x00116100
     */
    virtual int Type();

    /**
     * Report this message's class name.
     *
     * @return The literal `InvalidateSeekerMsg`.
     * @ghidraAddress 0x00116110
     */
    virtual const char *Name();

public:
    // Public because Voxer::HandleMessage(), Scratcher::HandleMessage(), and
    // NotePitcher::HandleMessage() reads these directly, through a InvalidateSeekerMsg pointer from
    // outside the hierarchy, and the image exposes no accessor. A friend declaration fits equally
    // well.
    int mUnknown04; // +0x04
    int mUnknown08; // +0x08
};

/**
 * Identity that InvalidateSeekerMsg::Type() reports.
 *
 * This word belongs to InvalidateSeekerMsg because InvalidateSeekerMsg::Type() at `0x00116100`
 * returns it, and the registration at `0x003d9818` passes the same value, 317, as the identity of
 * this class's factory.
 *
 * @ghidraAddress 0x006d0274
 */
extern int g_nInvalidateSeekerMsgType;
