#pragma once

#include <iostream>

#include "sch/command.h"

class GenericSequencer;

/**
 * Scheduler command that runs a sequencer's next dispatch.
 *
 * `12SequencerCmd` in the RTTI descriptor at `0x008ef260`, deriving publicly from Sch::Command at
 * offset 0. Its type function is at `0x00100b00` and its table at `0x007cc838` retains
 * Sch::Command::Save() and Load(). Sequencer::ScheduleNext() allocates the 0x10-byte object with
 * the untagged allocator and expands the constructor. The destructor at `0x00100ad8` is implicitly
 * declared.
 */
class SequencerCmd : public Sch::Command {
public:
    /**
     * Point the command at its sequencer.
     *
     * @param pOwner The sequencer to run.
     */
    explicit SequencerCmd(GenericSequencer *pOwner) : mOwner(pOwner) {
    }

    /**
     * Report sCmdID.
     *
     * @return The class's command identifier.
     * @ghidraAddress 0x00100b50
     */
    virtual int CmdID();

    /**
     * Run GenericSequencer::Dispatch() on the owner.
     *
     * @ghidraAddress 0x00100b60
     */
    virtual void Execute();

    /**
     * Write `{Sequencer}`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x00100b90
     */
    virtual void Print(std::ostream &stream);

    /**
     * The class's command identifier, which the image initialises to zero.
     *
     * @ghidraAddress 0x00675e50
     */
    static int sCmdID;

private:
    GenericSequencer *mOwner;
};
