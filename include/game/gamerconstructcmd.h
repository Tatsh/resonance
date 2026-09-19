#pragma once

#include <iostream.h>

#include "game/gamer.h"
#include "sch/command.h"

/**
 * Scheduler command that builds a gamer.
 *
 * `GamerConstructCmd` is attested by its own type-info accessor at `0x00116b70`, which the RTTI
 * harvest has no entry for, and by the literal `{Gamer}` at `0x007ce588` that its Print() writes.
 *
 * Its table at `0x007ce600` has eight entries. This class supplies the destructor and slots 3, 4,
 * and 5, and inherits slots 2, 6, and 7, the last two being `Sch::Command::Save` at `0x00539f20`
 * and `Load` at `0x00539f28`, which is what identifies the base.
 *
 * `Sch::Command` is 0x0c bytes, so this class's own two members start at `+0x0c`.
 *
 * CmdID() reports the word at `0x00669cb8`, which is zero, and `Sch::Command` documents a zero
 * result as marking the command unstreamable. So this command is never saved or loaded despite
 * inheriting both routines.
 */
class GamerConstructCmd : public Sch::Command {
public:
    /** @ghidraAddress 0x00116b48 */
    virtual ~GamerConstructCmd();

    /**
     * Report the identifier this class streams itself under.
     *
     * @return The word at `0x00669cb8`, which is zero.
     * @ghidraAddress 0x00116bc0
     */
    virtual int CmdID();

    /**
     * Build the gamer.
     *
     * Not reconstructed. It passes mArgument to the routine at `0x00111fa8` with mGamer as the
     * receiver, and that routine is an unnamed member of `Gamer` in the same band.
     *
     * @ghidraAddress 0x00116bd0
     */
    virtual void Execute();

    /**
     * Write this command's description to stream.
     *
     * Writes the single literal `{Gamer}` and nothing else, so the description names the class it
     * builds rather than any of its own state.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00116bf0
     */
    virtual void Print(ostream &stream);

private:
    Gamer *mGamer; // +0x0c the receiver Execute calls
    int mArgument; // +0x10 the value Execute passes
};

/**
 * Identifier GamerConstructCmd::CmdID() reports, which is zero.
 *
 * @ghidraAddress 0x00669cb8
 */
extern int g_nGamerConstructCmdID;
