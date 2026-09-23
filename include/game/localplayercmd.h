#pragma once

#include <iostream>

#include "sch/command.h"

class IBStream;
class LocalPlayer;
class OBStream;

/**
 * Scheduler command that runs a local player's per-bar update.
 *
 * `LocalPlayerCmd` is one of the five ordinary Sch::Command subclasses. Its table is at
 * `0x007cff18` with eight entries, and it overrides Save() and Load() with empty bodies of its own.
 * LocalPlayer allocates the 0x14-byte object with the untagged allocator and expands the
 * constructor, in LocalPlayer::Slot11() for the first bar and in LocalPlayer::OnBarTick() for
 * every bar after.
 */
class LocalPlayerCmd : public Sch::Command {
public:
    /**
     * Prepare the update of one bar.
     *
     * @param pPlayer The player to update.
     * @param nTick The song position the command runs at.
     */
    LocalPlayerCmd(LocalPlayer *pPlayer, int nTick) : mPlayer(pPlayer), mTick(nTick) {
    }

    /** @ghidraAddress 0x001227a8 */
    virtual ~LocalPlayerCmd();

    /**
     * Report sCmdID.
     *
     * @return The class's command identifier.
     * @ghidraAddress 0x00122840
     */
    virtual int CmdID();

    /**
     * Run LocalPlayer::OnBarTick() for the tick.
     *
     * @ghidraAddress 0x00122820
     */
    virtual void Execute();

    /**
     * Write `{LocalPlayerCmd}`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00122860
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write nothing.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00122850
     */
    virtual void Save(OBStream &stream);

    /**
     * Read nothing.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x00122858
     */
    virtual void Load(IBStream &stream);

    /**
     * The class's command identifier, which the image initialises to zero.
     *
     * @ghidraAddress 0x0066c508
     */
    static int sCmdID;

private:
    LocalPlayer *mPlayer; // +0x0c
    int mTick;            // +0x10
};
