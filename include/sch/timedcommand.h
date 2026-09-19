#pragma once

#include <iostream>

#include "app/attachment.h"
#include "sch/cmdid.h"
#include "sch/tick.h"

namespace Sch {
class Command;
}
class IBStream;
class OBStream;

namespace Sch {

/**
 * One queued command, together with the time the scheduler is to run it at.
 *
 * `Q23Sch12TimedCommand` in the RTTI descriptor at `0x008ef760`, deriving publicly from Attachment
 * at offset 0. Despite the shared prefix of the two titles, this class does not derive from
 * Sch::Command; the two are siblings under Attachment, and this one refers to a command rather than
 * being one.
 *
 * The vtable at `0x00835f08` runs three entries and then a zero entry. The class therefore
 * declares one virtual of its own:
 *
 *  - 0, the compiler-generated type function at `0x005d31f0`
 *  - 1, the destructor at `0x005d33e8`
 *  - 2, Attachment::Destroy() at `0x004bfea8`, inherited
 *
 * Run(), Print(), Save(), and Load() are therefore ordinary members rather than virtuals, and all
 * four are dispatched by a direct call.
 *
 * An instance is 0x28 bytes. The allocation at `0x004a6284` and the seven writes of the
 * constructor at `0x005d32f8` agree on the size. Every byte is accounted for and the layout
 * closes, with no reserved run and no padding beyond the natural alignment of the two 8-byte
 * members.
 *
 * Print() at `0x005d30b0` recovers the original word for four members. It writes `[`, then
 * mDueTick, then the literal ` local:` at `0x00835ef0`, then mLocalTick, then either ` abs` at
 * `0x00835ee0` or ` delta` at `0x00835ee8`, then ` id:` at `0x00835ef8`, then mCmdID, then a space,
 * then the command, then `]`. Those four literals are what titles mDueTick, mLocalTick, mDelta,
 * and mCmdID; mOrder alone is titled from behaviour, because Print() does not write it.
 */
class TimedCommand : public Attachment {
public:
    /**
     * Wrap a command, taking a reference to it.
     *
     * The constructor writes mOrder, mDueTick, and mCmdID with -1. A wrapper that the scheduler
     * has not yet queued therefore reports no due time, no order, and no handle.
     *
     * The reference is taken by incrementing the command's Attachment reference count directly at
     * `0x005d3338`, because the image declares no AddRef. Reading and writing the count of an
     * object whose class does not derive from this one is what makes `Attachment::mRefs` public.
     *
     * The tick argument lands at mLocalTick rather than at mDueTick, and mDueTick starts at -1.
     * The scheduler resolves the due time when it queues the wrapper.
     *
     * @param pCommand The command to run, or null.
     * @param tick The tick the caller requested, in the caller's own frame.
     * @param bDelta Non-zero when the requested tick is a distance from now rather than an absolute
     *               scheduler time.
     * @ghidraAddress 0x005d32f8
     */
    TimedCommand(Command *pCommand, Tick tick, int bDelta);

    /**
     * Give back the reference to the command and release the wrapper.
     *
     * @ghidraAddress 0x005d33e8
     */
    virtual ~TimedCommand();

    /**
     * Run the wrapped command.
     *
     * The body dispatches Sch::Command::Execute() and does nothing else. The title is inferred
     * from that body, and the scheduler run loop at `0x004aae68` is the one caller.
     *
     * @ghidraAddress 0x005d33b8
     */
    void Run();

    /**
     * Write a description of the wrapper to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x005d30b0
     */
    void Print(std::ostream &stream);

    /**
     * Write the wrapper and the command it refers to.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x005d3440
     */
    void Save(OBStream &stream);

    /**
     * Read the wrapper and the command it refers to back.
     *
     * The order matches Save() field for field.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x005d34d0
     */
    void Load(IBStream &stream);

    /**
     * The command to run, or null.
     *
     * Public because the scheduler dereferences it directly. The deferred queueing path at
     * `0x004ac764` marks the command as queued through it, and the run loop at `0x004aae84` clears
     * that mark the same way.
     *
     * +0x08
     */
    Command *mCommand;

    /**
     * Order among the wrappers that share one due tick.
     *
     * The scheduler orders its queue by mDueTick first and by this member second, comparing it as
     * an unsigned quantity at `0x004ac53c` and `0x004ac5a0`. A smaller value therefore runs
     * earlier, and the -1 the constructor writes sorts last. Every queueing call site in the image
     * supplies -1, at `0x004a6034`, `0x004a6104`, `0x004a61dc`, `0x004a62c0`, `0x004a63b0`,
     * `0x004a792c`, and `0x004a7958`, so the second key never changes the order in the shipped
     * build. Public because the two queueing paths write it, at `0x004ac62c` and `0x004ac70c`.
     *
     * The title is inferred from the comparison alone, because Print() does not write this member
     * and no literal in the image identifies it.
     *
     * +0x0c
     */
    int mOrder;

    /**
     * Scheduler time the command is to run at.
     *
     * Public because the scheduler writes it at `0x004ac634` and `0x004ac710`, the run loop
     * compares it against the clock at `0x004aa908` and copies it into the scheduler's current time
     * at `0x004aae64`, and the queue's ordering predicate reads it at `0x004ac518`. The comparison
     * stops the loop as soon as the earliest entry is still in the future.
     *
     * +0x10
     */
    Tick mDueTick;

    /**
     * The tick the caller requested, in the caller's own clock frame.
     *
     * Print() labels this member `local:`, which is what distinguishes it from mDueTick. The
     * absolute queueing path at `0x004ac634` subtracts the requesting clock's origin to produce
     * mDueTick, and the delta path at `0x004ac700` adds the scheduler's current time instead, so
     * the two differ whenever the requesting clock has an origin of its own. Only the constructor
     * writes this member, and only Print(), Save(), and Load() read it.
     *
     * +0x18
     */
    Tick mLocalTick;

private:
    // Non-zero when the requested tick is a distance from now rather than an absolute scheduler
    // time. Print() writes " delta" for a non-zero value and " abs" for zero, and
    // Sch::TickClock::Post() at 0x004a7924 branches on the same argument to choose between the two
    // queueing paths. Save() writes only the low byte. No code outside this class reads it. +0x20
    int mDelta;

public:
    /**
     * Handle the queueing request was made under.
     *
     * The constructor writes -1 and both queueing paths replace it with the caller's handle, at
     * `0x004ac660` and `0x004ac73c`, allocating a fresh value first when the caller's handle is
     * still -2. Public because the scheduler writes it.
     *
     * +0x24
     */
    CmdID mCmdID;
};

} // namespace Sch
