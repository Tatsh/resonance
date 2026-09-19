#pragma once

#include "memcard/memcardconnectstate.h"
#include "memcard/memcardtask.h"

/**
 * Enquire about one card slot and report what it holds.
 *
 * `18GetConnectStateMCT` in the RTTI descriptor at `0x008ef6a0`, single inheritance from
 * `MemcardTask` at offset 0. An instance is 0x34 bytes and the vtable is at `0x007dad98`.
 *
 * The task is the thinnest of the family. It queues one `CheckInfo`, translates the result into
 * mConnectState, and reports that record by value. The slot's display text comes from
 * g_apszMemcardSlotNames, chosen by matching mPortSlot against g_anMemcardSlotPortSlot, and the
 * search falls back on `sceMcGetSlotMax()` to decide whether a port has a multi-tap.
 */
class GetConnectStateMCT : public MemcardTask {
public:
    /**
     * Construct an idle enquiry with an unknown state.
     *
     * The record starts with its port and slot, its free count and its type all -1 and its
     * formatted flag zero, and its display text an empty string rather than a null one.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param pCookie The tag that abandons exactly this task's operations.
     * @ghidraAddress 0x001848f8
     */
    GetConnectStateMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie);

    /** @ghidraAddress 0x001849c8 */
    virtual ~GetConnectStateMCT();

    /**
     * Fill mConnectState from the finished enquiry and report it.
     *
     * The formatted flag is recorded as the result of comparing the operation's own flag against
     * exactly 1, so any other non-zero value arrives as zero.
     *
     * @ghidraAddress 0x00177fb0
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /** @ghidraAddress 0x00185ff0 */
    virtual void Finish();

    /** @ghidraAddress 0x00186070 */
    virtual void Execute();

private:
    // What the slot reported. Passed to the user by value, which is why MemcardUser's slot 2 is
    // the one slot whose compiled body is not empty. +0x1c
    MemcardConnectState mConnectState;
};
