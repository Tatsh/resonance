#pragma once

#include <iostream>

class IBStream;
class OBStream;

namespace Sch {

/**
 * Handle that identifies one batch of commands the scheduler has queued.
 *
 * The class is not polymorphic and emits no RTTI. Its Print() writes the literal ` {cmdID ` at
 * `0x008379f0`. It is unrelated to Sch::Command::CmdID(). Sch::Command::CmdID() reports the class
 * a command streams itself under, and this handle identifies a queueing request.
 *
 * The object is four bytes. Save() at `0x005e59a0` reads one four-byte word at offset 0 and writes
 * it, Load() at `0x005e59e0` reads four bytes back into the same word, and Print() at `0x005e5958`
 * writes that word through one integer insertion. No instruction in the image touches any other
 * offset.
 *
 * A caller hands the scheduler a pointer to one of these. The three queueing paths at
 * `0x004ac644`, `0x004ac720`, and `0x004ac85c` treat -2 as "not yet allocated", replace it with a
 * fresh value from AllocateValue(), and copy the result into the wrapper they queue. Several
 * commands queued through the same handle therefore share one value. The cancellation path at
 * `0x004a79d0` withdraws only the first queued wrapper with a matching value, because
 * Sch::Scheduler::WithdrawByCmdID() stops at the first match.
 *
 * The one member is public, because the queueing paths assign it directly and the image exposes no
 * accessor.
 *
 * `Sch::Tick` is streamed and printed through the same three-function shape, at `0x006100a8`,
 * `0x00610118`, and `0x00610050`. The two types are unrelated beyond that shape.
 */
class CmdID {
public:
    /**
     * Write the handle.
     *
     * @param stream The stream to write to.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress NTSC-U/C: 0x005e59a0
     * @ghidraAddress PAL: 0x00627b60
     */
    OBStream &Save(OBStream &stream);

    /**
     * Read the handle back.
     *
     * @param stream The stream to read from.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress NTSC-U/C: 0x005e59e0
     * @ghidraAddress PAL: 0x00627ba0
     */
    IBStream &Load(IBStream &stream);

    /**
     * Write a description of the handle to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x005e5958
     * @ghidraAddress PAL: 0x00627b18
     */
    void Print(std::ostream &stream);

    /**
     * Produce the next handle value that no replayed recording has reserved.
     *
     * The counter at `0x0077d2e8` advances past every reserved value it meets. The reserved values
     * are the set at `0x008e4f08`, which a replayed recording fills and whose cursor at
     * `0x008e4f18` walks forward alongside the counter. The name is inferred.
     *
     * @return The value.
     * @ghidraAddress NTSC-U/C: 0x005e4dc8
     * @ghidraAddress PAL: 0x00626f88
     */
    static int AllocateValue();

    /**
     * Reserve a handle value so that AllocateValue() does not hand it out.
     *
     * The value joins the reserved set, and the set's cursor returns to its first entry.
     * Sch::Playbacker::Load() calls this for every wrapper it reads back. The handles of a
     * replayed recording therefore remain unique. The handle arrives by value, as a copy the
     * caller builds.
     *
     * @param id The handle to reserve.
     * @ghidraAddress NTSC-U/C: 0x005e5908
     * @ghidraAddress PAL: 0x00627ac8
     */
    static void ReserveID(CmdID id);

    /**
     * The value, or -2 while no value has been allocated.
     *
     * +0x00
     */
    int mValue;
};

} // namespace Sch
