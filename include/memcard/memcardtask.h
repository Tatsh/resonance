#pragma once

#include "memcard/memcardcbhandler.h"

class Memcard;
class MemcardUser;

/** MemcardTask::mState before Execute() has run. */
constexpr int kMemcardTaskIdle = 0;

/** MemcardTask::mState while the task is queueing and receiving operations. */
constexpr int kMemcardTaskRunning = 1;

/** MemcardTask::mState once Finish() has reported to the user. */
constexpr int kMemcardTaskFinished = 2;

/**
 * One multi-step piece of memory-card work, driven by the operations it queues.
 *
 * `11MemcardTask` in the RTTI descriptor at `0x008ef670`, single inheritance from
 * `MemcardCBHandler` at offset 0. An instance is 0x1c bytes. No vtable for this class is emitted
 * anywhere in the image, which is what establishes that it is abstract, and the two virtuals every
 * subclass overrides are therefore declared pure here.
 *
 * A task is a `MemcardCBHandler` and therefore receives the completion of every operation it
 * queues. It drives itself forward from inside those reports. `SaveFileMCT` for example creates the
 * save directory, then opens the file, then writes it, then closes it, advancing one step per
 * report. The task finally reports to the `MemcardUser` it was constructed with.
 *
 * Sixteen concrete subclasses exist. `SaveFileMCT` and `LoadFileMCT` perform the file transfer,
 * and the fourteen others either drive the card directly or wrap one of those two.
 *
 * The method titles Execute() and Finish() are inferred. The diagnostic `MemcardTask::Execute()`
 * at `0x007da088` attests the class and the method together, but the routine that prints it has
 * not been located and no string identifies Finish().
 */
class MemcardTask : public MemcardCBHandler {
public:
    /**
     * Construct an idle task against one card slot.
     *
     * mStatus is not written. A task that is read before it reports therefore exposes whatever
     * that word stored when the block was allocated. The compiler inlined this body into every
     * subclass constructor, and no address of its own survives.
     * `GetConnectStateMCT::GetConnectStateMCT()` at `0x001848f8` is the clearest copy.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param pCookie The tag that abandons exactly this task's operations.
     */
    MemcardTask(MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie);

    /**
     * Report the finished task to its user and record that it is done.
     *
     * Occupies vtable slot 16. Every subclass writes 2 to mState, or its own state member, and
     * then calls the one MemcardUser method that matches the task.
     */
    virtual void Finish() = 0;

    /**
     * Start the task.
     *
     * Occupies vtable slot 17. Every subclass writes 1 to mState, or its own state member, and
     * then queues its first operation on mCard.
     */
    virtual void Execute() = 0;

    /**
     * Unrecovered. Slot 18.
     *
     * The body is empty and every subclass in the image inherits it. Neither its purpose nor its
     * argument list can be established. It is declared without arguments because no call site
     * exists to prove any.
     *
     * @ghidraAddress 0x00184530
     */
    virtual void OnUnknown18();

protected:
    /**
     * Abandon the task when the last operation reported a failure.
     *
     * An mStatus other than kMemcardStatusOk abandons every operation still queued under mCookie
     * and then reports through Finish(). kMemcardStatusOk does nothing at all. Nothing in the
     * image calls the out-of-line copy, every report handler across the sixteen subclasses having
     * inlined the body instead.
     *
     * @ghidraAddress 0x00185998
     */
    void AbortOnError();

    // The receiver Finish() reports to. +0x04
    MemcardUser *mUser;

    // The queue this task submits operations to. +0x08
    Memcard *mCard;

    // The tag every queued operation records. Memcard::Cancel() therefore abandons this task's
    // work and nothing else. +0x0c
    void *mCookie;

    // The packed port and slot. +0x10
    int mPortSlot;

    // Zero while idle, 1 once Execute() has run, and 2 once Finish() has run. SaveFileMCT and
    // LoadFileMCT never write it, because each of those two maintains a state member of its own
    // instead. +0x14
    int mState;

    // One of MemcardStatus, copied from the operation that last reported. +0x18
    int mStatus;
};
