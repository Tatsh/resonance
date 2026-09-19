#pragma once

#include <iostream>

class IBStream;
class OBStream;

/**
 * Handle that identifies one batch of commands the scheduler has queued.
 *
 * The class is not polymorphic and emits no RTTI, and it has no embedded file name, so its title
 * is inferred from the literal ` {cmdID ` at `0x008379f0` that its own Print() writes. The same
 * evidence route produced Sch::Command::CmdID(); the two are unrelated, because that slot reports
 * the class a command streams itself under while this handle identifies a queueing request.
 *
 * The object is four bytes. Save() at `0x005e59a0` reads one four-byte word at offset 0 and writes
 * it, Load() at `0x005e59e0` reads four bytes back into the same word, and Print() at `0x005e5958`
 * writes that word through one integer insertion. No instruction in the image touches any other
 * offset.
 *
 * A caller hands the scheduler a pointer to one of these. The three queueing paths at
 * `0x004ac644`, `0x004ac720`, and `0x004ac85c` treat -2 as "not yet allocated", replace it with a
 * fresh value from the allocator at `0x005e4dc8`, and copy the result into the wrapper they queue.
 * Several commands queued through the same handle therefore share one value, and the cancellation
 * path at `0x004a79d0` withdraws every wrapper with a matching value in one call. That allocator
 * maintains a counter at `0x0077d2e8` and a container at `0x008e4f08` outside this class, so it is
 * not reconstructed here.
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
     * @ghidraAddress 0x005e59a0
     */
    OBStream &Save(OBStream &stream);

    /**
     * Read the handle back.
     *
     * @param stream The stream to read from.
     * @return The stream, allowing calls to be chained.
     * @ghidraAddress 0x005e59e0
     */
    IBStream &Load(IBStream &stream);

    /**
     * Write a description of the handle to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x005e5958
     */
    void Print(std::ostream &stream);

    /**
     * The value, or -2 while no value has been allocated.
     *
     * +0x00
     */
    int mValue;
};
