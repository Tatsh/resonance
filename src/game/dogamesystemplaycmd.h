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
 * `0x0010b530` registers New() under identifier 4 with the factory registrar.
 *
 * The destructor at `0x0010bd08` is implicitly declared.
 */
class DoGameSystemPlayCmd : public Sch::Command {
public:
    /**
     * Produce a command on the heap.
     *
     * @return The command.
     * @ghidraAddress NTSC-U/C: 0x00105e40
     * @ghidraAddress PAL: 0x00105e40
     */
    static Sch::Command *New();

    /**
     * Report sCmdID.
     *
     * @return The class's command identifier.
     * @ghidraAddress NTSC-U/C: 0x0010bdb8
     * @ghidraAddress PAL: 0x0010bf50
     */
    virtual int CmdID();

    /**
     * Run GameManagerImpl::StartPlay() on the application's game manager.
     *
     * @ghidraAddress NTSC-U/C: 0x0010bd80
     * @ghidraAddress PAL: 0x0010bf18
     */
    virtual void Execute();

    /**
     * Write `{DoGameSystemPlayCmd}`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x0010bdd8
     * @ghidraAddress PAL: 0x0010bf70
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write nothing.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x0010bdc8
     * @ghidraAddress PAL: 0x0010bf60
     */
    virtual void Save(OBStream &stream);

    /**
     * Read nothing.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x0010bdd0
     * @ghidraAddress PAL: 0x0010bf68
     */
    virtual void Load(IBStream &stream);

    /**
     * Identifier the class streams itself under. The word at `0x006682b8` starts as 4.
     *
     * @ghidraAddress NTSC-U/C: 0x006682b8
     * @ghidraAddress PAL: 0x006a8e38
     */
    static int sCmdID;
};
