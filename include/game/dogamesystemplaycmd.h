#pragma once

#include <iostream>

#include "sch/command.h"

class IBStream;
class OBStream;

/**
 * Scheduler command that starts a play of the game system.
 *
 * `DoGameSystemPlayCmd` is one of the five ordinary Sch::Command subclasses. Its table is at
 * `0x007cd520` with eight entries, and it overrides Save() and Load() with empty bodies of its own.
 * It carries no payload, so the object is the 0x0c-byte base. The static initialiser at
 * `0x0010b530` registers New() under identifier 4 with the factory registrar, which is not
 * reconstructed.
 *
 * The destructor at `0x0010bd08` is implicitly declared.
 */
class DoGameSystemPlayCmd : public Sch::Command {
public:
    /**
     * Produce a command on the heap.
     *
     * @return The command.
     * @ghidraAddress 0x00105e40
     */
    static Sch::Command *New();

    /**
     * Report sCmdID.
     *
     * @return The class's command identifier.
     * @ghidraAddress 0x0010bdb8
     */
    virtual int CmdID();

    /**
     * Run GameManagerImpl::OnUnknownSlot6() on the application's game manager.
     *
     * @ghidraAddress 0x0010bd80
     */
    virtual void Execute();

    /**
     * Write `{DoGameSystemPlayCmd}`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0010bdd8
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write nothing.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0010bdc8
     */
    virtual void Save(OBStream &stream);

    /**
     * Read nothing.
     *
     * @param stream The stream to read from.
     * @ghidraAddress 0x0010bdd0
     */
    virtual void Load(IBStream &stream);

    /**
     * Identifier the class streams itself under. The word at `0x006682b8` starts as 4.
     *
     * @ghidraAddress 0x006682b8
     */
    static int sCmdID;
};
