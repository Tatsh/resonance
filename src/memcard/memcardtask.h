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
 * Its RTTI descriptor is at `0x008ef670`. It has single inheritance from `MemcardCBHandler` at
 * offset 0. An instance is 0x1c bytes. No vtable for this class is emitted anywhere in the image.
 * The class is therefore abstract, and the two virtuals every subclass overrides are declared pure
 * here.
 *
 * A task is a `MemcardCBHandler` and therefore receives the completion of every operation it
 * queues. It drives itself forward from inside those reports. `SaveFileMCT` for example creates the
 * save directory, then opens the file, then writes it, then closes it, advancing one step per
 * report. The task finally reports to the `MemcardUser` it was constructed with.
 *
 * Sixteen concrete subclasses exist. `SaveFileMCT` and `LoadFileMCT` perform the file transfer,
 * and the fourteen others either drive the card directly or wrap one of those two. The European
 * release adds a seventeenth, `SaveSpaceCheckerMCT`.
 *
 * The method titles Execute() and Finish() are inferred. The diagnostic `MemcardTask::Execute()`
 * at `0x007da088` attests the class and the method together, but the routine that prints it has
 * not been located and no string identifies Finish(). In the European release,
 * SaveSpaceCheckerMCT::Execute() prints the same text.
 */
class MemcardTask : public MemcardCBHandler {
public:
    /**
     * Construct an idle task against one card slot.
     *
     * mStatus is not written. A task that is read before it reports therefore exposes whatever
     * that word stored when the block was allocated. The compiler inlined this body into every
     * subclass constructor, and the image has no out-of-line copy.
     * `GetConnectStateMCT::GetConnectStateMCT()` at `0x001848f8` is the clearest copy.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     */
    MemcardTask(MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie);

    /**
     * Construct an idle task that does not address a single card slot.
     *
     * mPortSlot and mStatus are not written. The only copy is inlined into
     * MemcardManager::CreateGetAllConnectStatesTask() at `0x001f2c70`, whose task enquires about
     * every slot from its tables.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nCookie The tag that abandons exactly this task's operations.
     */
    MemcardTask(MemcardUser *pUser, Memcard *pCard, int nCookie)
        : mUser(pUser), mCard(pCard), mCookie(nCookie), mState(kMemcardTaskIdle) {
    }

    /**
     * Report the finished task to its user and record that it is done.
     *
     * Occupies vtable slot 16. Every subclass writes 2 to mState, or its state member, and
     * then calls the one MemcardUser method that matches the task.
     */
    virtual void Finish() = 0;

    /**
     * Start the task.
     *
     * Occupies vtable slot 17. Every subclass writes 1 to mState, or its state member, and
     * then queues its first operation on mCard.
     */
    virtual void Execute() = 0;

#ifndef VIDEO_STANDARD_PAL
    /**
     * Do nothing.
     *
     * Slot 18. The body is empty, every subclass in the image inherits it, and the image never
     * calls it. It is declared without arguments because no call site exists to prove any. The
     * European release has no slot 18.
     *
     * @ghidraAddress NTSC-U/C: 0x00184530
     */
    virtual void UnusedHook();
#endif

    // MemcardManager::Update() reads mState to start an idle task and retire a finished one.
    friend class MemcardManager;

protected:
    /**
     * Abandon the task when the last operation reported a failure.
     *
     * An mStatus other than kMemcardStatusOk abandons every operation still queued under mCookie
     * and then reports through Finish(). kMemcardStatusOk does nothing. The out-of-line copy is
     * never called. Every report handler across the sixteen subclasses inlined the body instead.
     *
     * @ghidraAddress 0x00185998
     */
    void AbortOnError();

    // The receiver Finish() reports to. +0x04
    MemcardUser *mUser;

    // The queue this task submits operations to. +0x08
    Memcard *mCard;

    // The ticket number every queued operation records, drawn by MemcardManager's task factories
    // from MemcardManager::mTicket. Memcard::Cancel() therefore abandons only this task's work.
    // +0x0c
    int mCookie;

    // The packed port and slot. +0x10
    int mPortSlot;

    // Zero while idle, 1 once Execute() has run, and 2 once Finish() has run. SaveFileMCT and
    // LoadFileMCT never write it. Each of those two maintains a separate state member instead.
    // +0x14
    int mState;

    // One of MemcardStatus, copied from the operation that last reported. +0x18
    int mStatus;
};
