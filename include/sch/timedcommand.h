#pragma once

#include "app/attachment.h"
#include "sch/command.h"
#include "sch/tick.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

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
 * Run(), Save(), and Load() are therefore ordinary members rather than virtuals, and all three are
 * dispatched by a direct call.
 *
 * An instance is 0x28 bytes. The allocation at `0x004a6284` and the seven writes of the
 * constructor at `0x005d32f8` agree on the size. Every byte is accounted for and the layout
 * closes, with no reserved run and no padding beyond the natural alignment of the two 8-byte
 * members.
 *
 * Four of the six member titles are undetermined. The scheduler is the only code outside this
 * class that touches any of them, and it writes four of them directly with no accessor in the
 * image. Those four are public for that reason.
 */
class TimedCommand : public Attachment {
public:
    /**
     * Wrap a command, taking a reference to it.
     *
     * The constructor writes mUnknown0c, mDueTick, and mUnknown24 with -1. A wrapper that the
     * scheduler has not yet queued therefore reports no due time and no serial.
     *
     * The reference is taken by incrementing the command's Attachment reference count directly at
     * `0x005d3338`, because the image declares no AddRef. Reading and writing the count of an
     * object whose class does not derive from this one is what makes `Attachment::mRefs` public.
     *
     * The tick argument lands at mUnknown18 rather than at mDueTick, and mDueTick starts at -1. The
     * scheduler computes the due tick when it queues the wrapper.
     *
     * @param pCommand The command to run, or null.
     * @param tick The tick the caller requested.
     * @param nUnknown20 A flag the caller passes through. Every call site passes either 0 or a
     *                   value forwarded from its own caller.
     * @ghidraAddress 0x005d32f8
     */
    TimedCommand(Command *pCommand, Tick tick, int nUnknown20);

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
     * from that body, and the scheduler run loop at `0x004aa848` is the one caller.
     *
     * @ghidraAddress 0x005d33b8
     */
    void Run();

    /**
     * Write the wrapper and the command it refers to.
     *
     * The body is not reconstructed yet, because the 4-byte member at `+0x24` is streamed by the
     * pair at `0x005e59a0` and `0x005e59e0`, whose owning type is not recovered. The recovered
     * order is mDueTick through the pair at `0x006100a8`, then mUnknown0c as four bytes through
     * OBStream::Write(), then mUnknown18 through the same tick pair, then the low byte of
     * mUnknown20, then mUnknown24, and finally mCommand through `Sch::operator<<`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x005d3440
     */
    void Save(OBStream &stream);

    /**
     * Read the wrapper and the command it refers to back.
     *
     * The body is not reconstructed yet, for the reason recorded on Save(). The order matches
     * Save() field for field, through `0x00610118`, IBStream::Read(), `0x00610118`, `0x004edb78`,
     * `0x005e59e0`, and `Sch::operator>>`.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x005d34d0
     */
    void Load(IBStream &stream);

    /**
     * The command to run, or null.
     *
     * Public because the scheduler dereferences it directly. The deferred queueing path at
     * `0x004ac764` marks the command as queued through it, and the run loop at `0x004aae70` clears
     * that mark the same way.
     *
     * +0x08
     */
    Command *mCommand;

    /**
     * Undetermined. The constructor writes -1 and both queueing paths overwrite it, at
     * `0x004ac62c` and `0x004ac70c`, with a value the caller supplies. Every call site supplies
     * 0xffffffff. The field therefore stays at -1 throughout the shipped image. Public because
     * the scheduler writes it.
     *
     * +0x0c
     */
    int mUnknown0c;

    /**
     * The tick the scheduler is to run the command at.
     *
     * Public because the scheduler writes it at `0x004ac634` and `0x004ac710` and the run loop
     * compares it against the clock at `0x004aa878`. The title is inferred from that comparison.
     * The comparison stops the loop as soon as the earliest entry is still in the future.
     *
     * +0x10
     */
    Tick mDueTick;

    /**
     * Undetermined. The constructor writes the tick argument here, and no code outside this class
     * touches it.
     *
     * +0x18
     */
    Tick mUnknown18;

private:
    // Undetermined. The constructor writes the int argument here, and Save() writes only its low
    // byte. No code outside this class touches it. +0x20
    int mUnknown20;

public:
    /**
     * Undetermined. The constructor writes -1, and both queueing paths replace it with a fresh
     * serial from the allocator at `0x005e4dc8`. Public because the scheduler writes it.
     *
     * +0x24
     */
    int mUnknown24;
};

} // namespace Sch
