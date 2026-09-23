#pragma once

#include <iostream>

#include "game/gamer.h"
#include "sch/command.h"

/**
 * Scheduler command that runs a gamer's per-bar update.
 *
 * The class has its own type-info accessor at `0x00116b70`, which the RTTI harvest has no entry
 * for, so the name is inferred from what Execute() does. Print() writes the literal `{Gamer}` at
 * `0x007ce588`.
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
class GamerCmd : public Sch::Command {
public:
    /**
     * Prepare the update of one bar.
     *
     * Inline. Gamer::ScheduleBar() expands it after allocating the command at `0x001128f8`.
     *
     * @param pGamer The gamer to update.
     * @param nBar The bar.
     */
    GamerCmd(Gamer *pGamer, int nBar) : mGamer(pGamer), mBar(nBar) {
    }

    /** @ghidraAddress 0x00116b48 */
    virtual ~GamerCmd();

    /**
     * Report the identifier this class streams itself under.
     *
     * @return The word at `0x00669cb8`, which is zero.
     * @ghidraAddress 0x00116bc0
     */
    virtual int CmdID();

    /**
     * Run Gamer::OnBar() for the bar.
     *
     * @ghidraAddress 0x00116bd0
     */
    virtual void Execute();

    /**
     * Write this command's description to stream.
     *
     * Writes the single literal `{Gamer}` and nothing else.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00116bf0
     */
    virtual void Print(std::ostream &stream);

private:
    Gamer *mGamer; // +0x0c
    int mBar;      // +0x10
};

/**
 * Identifier GamerCmd::CmdID() reports, which is zero.
 *
 * @ghidraAddress 0x00669cb8
 */
extern int g_nGamerCmdID;
