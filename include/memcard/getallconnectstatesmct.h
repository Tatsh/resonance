#pragma once

#include <vector>

#include "memcard/memcardconnectstate.h"
#include "memcard/memcardtask.h"

/** Slots mSlotIndex reserves room for, which is two ports of four multi-tap slots. */
constexpr int kMemcardMaxEnumeratedSlots = 8;

/**
 * Enquire about every slot a multi-tap makes available and collect the answers.
 *
 * `22GetAllConnectStatesMCT` in the RTTI descriptor at `0x008ef6b0`, single inheritance from
 * `MemcardTask` at offset 0. An instance is 0x48 bytes and the vtable is at `0x007dacf8`.
 *
 * Execute() queues one `CheckInfo` per slot, decides how many from `sceMcGetSlotMax()`, and
 * records the expected count. Each report appends one MemcardConnectState to the vector the task
 * was given and increments the completed count, and the last of them finishes the task. The
 * MemcardTask::mPortSlot the task was constructed with is not used, because the slots come from
 * the tables instead.
 *
 * Two faithful oddities. A multi-tap on port 1 yields four enquiries, and the same tap on port 2
 * yields none at all rather than four, which leaves the last four entries of both slot tables
 * unreferenced by any code path. And the vector is borrowed rather than owned, which the
 * destructor confirms by releasing nothing.
 */
class GetAllConnectStatesMCT : public MemcardTask {
public:
    /**
     * Construct an idle enquiry.
     *
     * The constructor is inlined into MemcardManager::CreateGetAllConnectStatesTask() at
     * `0x001f2c70`, its one site, and no address of its own survives. The site writes no port slot
     * and clears mExpected and mCompleted.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param pStates Where the answers are appended. Borrowed, not owned.
     */
    GetAllConnectStatesMCT(MemcardUser *pUser,
                           Memcard *pCard,
                           int nCookie,
                           std::vector<MemcardConnectState> *pStates);

    /**
     * Release the task.
     *
     * The body releases nothing, which is what establishes that mStates is borrowed.
     *
     * @ghidraAddress 0x00184b08
     */
    virtual ~GetAllConnectStatesMCT();

    /**
     * Append one slot's state to mStates and finish once every enquiry has reported.
     *
     * kMemcardStatusUnknown skips the append but still counts toward the total, so a missing card
     * does not stall the task.
     *
     * @ghidraAddress 0x00178138
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /** @ghidraAddress 0x001860a0 */
    virtual void Finish();

    /**
     * Queue one enquiry per available slot.
     *
     * @ghidraAddress 0x001860d8
     */
    virtual void Execute();

private:
    // Enquiries Execute() queued, which is what mCompleted is compared against. +0x1c
    int mExpected;

    // Where the answers are appended. Borrowed, not owned. +0x20
    std::vector<MemcardConnectState> *mStates;

    // Index into the slot tables for each queued enquiry, in the order they were queued. +0x24
    int mSlotIndex[kMemcardMaxEnumeratedSlots];

    // Enquiries that have reported. +0x44
    int mCompleted;
};
