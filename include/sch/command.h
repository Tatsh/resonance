#pragma once

#include <iostream.h>

#include "app/attachment.h"
#include "stream/ibstream.h"
#include "stream/obstream.h"

namespace Sch {

/**
 * Base of every action the scheduler performs on behalf of another subsystem.
 *
 * `Q23Sch7Command` in the RTTI descriptor at `0x008f0950`, deriving publicly from Attachment at
 * offset 0. The base subobject supplies the reference count at `+0x00` and the vtable pointer at
 * `+0x04`, this class adds one word at `+0x08`, and an instance is 0x0c bytes. Every derived class
 * starts its own data at `+0x0c`. Three independent measurements agree on that boundary. The
 * inlined construction at `0x0013a9a8` allocates 0x10 bytes and writes the reference count, the
 * word at `+0x08`, the vtable pointer, and one derived pointer at `+0x0c`; `ExitCmd::Save()` at
 * `0x00194a50` streams `+0x0c`, `+0x10`, and `+0x14`; and `GsPeriodical::PeriodicalCmd::Execute()`
 * at `0x001b4820` reads `+0x0c` and `+0x10`. The layout closes arithmetically with no unaccounted
 * byte.
 *
 * The vtable at `0x008288c0` runs eight entries and then a zero entry. The interface is therefore
 * complete rather than partial:
 *
 *  - 0, the compiler-generated type function at `0x00539e88`
 *  - 1, the destructor at `0x00539ef8`
 *  - 2, Attachment::Destroy() at `0x004bfea8`, inherited
 *  - 3, CmdID(), the `__pure_virtual` handler at `0x005381a8`
 *  - 4, Execute(), the same handler
 *  - 5, Print() at `0x0053a088`
 *  - 6, Save() at `0x00539f20`
 *  - 7, Load() at `0x00539f28`
 *
 * Twenty-four classes derive from this one. Five are ordinary classes, `ControllerCmd`,
 * `SequencerCmd`, `LocalPlayerCmd`, `DoGameSystemPlayCmd`, and `EndRecordingCmd`. The other
 * nineteen are file-local, and their descriptors identify the translation unit that defines each
 * one, among them `AppTickTask.cpp`, `AppTimeTask.cpp`, `AppActiveFilter.cpp`, `CmdPostScript.cpp`,
 * `ForceFeedbackMgr.cpp`, `GsAutoRiffer.cpp`, `GsNotePlayer.cpp`, `GsPeriodical.cpp`, and
 * `GsPhraseMgr.cpp`. Both slot 3 and slot 4 are pure here because every one of the twenty-four
 * supplies its own body for each.
 *
 * Two of the six member titles are recovered and four are inferred. Save() and Load() follow from
 * their bodies, and both bodies stream one field after another in the same order through the two
 * stream interfaces. CmdID() follows from the static member the RTTI records as
 * `_13ControllerCmd$sCmdID` and from the two diagnostics `Streamed Command ID %ld` and `Cannot
 * find ID %ld in Command Factory List`; every slot-3 body returns one per-class word, and each of
 * those words is zero throughout the shipped image. Execute() and Print() are inferred from their
 * bodies alone, and no string in the image identifies either.
 *
 * Nothing registers a factory in the shipped build. The registrar constructor at `0x00538208`
 * receives a literal zero for the identifier at all twenty-seven of its call sites, and it returns
 * without touching the list whenever the identifier is zero. The factory list is therefore
 * permanently empty and every path behind it is unreachable. That registrar is not reconstructed
 * here, because the image records no title for its class and no RTTI descriptor exists for it.
 */
class Command : public Attachment {
public:
    /**
     * Construct a command that the scheduler has not queued.
     *
     * The compiler inlined this body into every derived constructor. No address of its own
     * survives. The write of zero to mQueued is visible at `0x0013a9c4` among others.
     */
    Command() : mQueued(0) {
    }

    /**
     * Release the command.
     *
     * The body restores this class's vtable pointer and runs the Attachment destructor, and it
     * touches no member of its own.
     *
     * @ghidraAddress 0x00539ef8
     */
    virtual ~Command();

    /**
     * Report the identifier this class streams itself under.
     *
     * Every derived body returns one per-class word. A zero result marks the command as
     * non-serializable, and `operator<<` reports that case through Fatal().
     *
     * @return The identifier.
     */
    virtual int CmdID() = 0;

    /**
     * Perform the action this command stands for.
     *
     * Sch::TimedCommand::Run() dispatches this slot and discards nothing, because every derived
     * body returns no value.
     */
    virtual void Execute() = 0;

    /**
     * Write a description of this command to a diagnostic stream.
     *
     * The body here writes the literal `{Command}` and each derived body writes its own literal.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0053a088
     */
    virtual void Print(ostream &stream);

    /**
     * Write this command's payload.
     *
     * The body here is a single `jr ra`. The base therefore does not write a payload.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00539f20
     */
    virtual void Save(OBStream &stream);

    /**
     * Read this command's payload back.
     *
     * The body here is a single `jr ra`. The base therefore does not read a payload.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00539f28
     */
    virtual void Load(IBStream &stream);

    /**
     * Produce a command of the identified class through the factory list.
     *
     * The compiler inlined this body into `operator>>`, and the standalone copy at the address
     * below has no caller in the image. An unregistered identifier produces a null result. Every
     * path in the shipped build then reports a null result through Fatal(), because the factory
     * list is always empty.
     *
     * @param nCmdID The identifier to construct for.
     * @return The new command, or null when the identifier is zero or unregistered.
     * @ghidraAddress 0x00539fe8
     */
    static Command *NewCommand(int nCmdID);

    /**
     * Non-zero once the scheduler has queued this command through its deferred path.
     *
     * Public because the scheduler writes it from outside the hierarchy and the image exposes no
     * accessor. The deferred queueing path writes 1 at `0x004ac768` and the run loop writes 0 at
     * `0x004aae70` as soon as Sch::TimedCommand::Run() has returned. The immediate queueing path
     * at `0x004ac608` does not write it at all. No instruction in the image reads the member. The
     * title is therefore inferred from the two writes rather than from a test.
     *
     * +0x08
     */
    int mQueued;
};

/**
 * Write a command through the stream, preceded by a presence byte.
 *
 * A null command produces the single byte `0`. A command whose CmdID() is zero is reported through
 * Print() and Fatal(). Any other command produces the byte `1`, then its identifier as four bytes,
 * then its own payload.
 *
 * @param stream The stream to write to.
 * @param pCommand The command to write, or null.
 * @return The stream, allowing calls to be chained.
 * @ghidraAddress 0x005384d0
 */
OBStream &operator<<(OBStream &stream, Command *pCommand);

/**
 * Read a command back through the stream and construct it from the factory list.
 *
 * The presence byte `0`, and an end of data before it arrives, both produce a null result. Any
 * byte other than `0` or `1` is reported through Fatal().
 *
 * @param stream The stream to read from.
 * @param pCommand Receives the new command, or null.
 * @return The stream, allowing calls to be chained.
 * @ghidraAddress 0x005385f0
 */
IBStream &operator>>(IBStream &stream, Command *&pCommand);

} // namespace Sch
