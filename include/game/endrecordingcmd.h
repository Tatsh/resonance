#pragma once

#include <iostream>

#include "sch/command.h"

class GameRecorder;
class OBStream;

/**
 * Scheduler command that ends a recording.
 *
 * `EndRecordingCmd` is one of the five ordinary Sch::Command subclasses. Its table at `0x007cdcb0`
 * has nine entries: it overrides Save() with an empty body of its own, inherits
 * Sch::Command::Load() at `0x00539f28`, and adds one virtual of its own at slot 8, which is empty.
 * GameRecorder::ScheduleEnd() allocates the 0x10-byte object with the untagged allocator and
 * expands the constructor. The static initialiser of the unit registers New() under identifier 6
 * with the factory registrar, which is not reconstructed.
 *
 * The destructor at `0x0010ef30` is implicitly declared.
 */
class EndRecordingCmd : public Sch::Command {
public:
    /**
     * Construct a command with no recorder, as New() does.
     */
    EndRecordingCmd() {
    }

    /**
     * Prepare the end of one recording.
     *
     * @param pRecorder The recorder to end.
     */
    explicit EndRecordingCmd(GameRecorder *pRecorder) : mRecorder(pRecorder) {
    }

    /**
     * Produce a command on the heap.
     *
     * @return The command.
     * @ghidraAddress 0x0010c8d0
     */
    static Sch::Command *New();

    /**
     * Report sCmdID.
     *
     * @return The class's command identifier.
     * @ghidraAddress 0x0010efc8
     */
    virtual int CmdID();

    /**
     * Run GameRecorder::EndRecording() on the recorder.
     *
     * @ghidraAddress 0x0010efa8
     */
    virtual void Execute();

    /**
     * Write `{EndRecordingCmd}`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0010efe8
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write nothing.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0010efd8
     */
    virtual void Save(OBStream &stream);

    /**
     * Slot 8, the one virtual this class adds. The body is empty.
     *
     * @ghidraAddress 0x0010efe0
     */
    virtual void Slot8();

    /**
     * Identifier the class streams itself under. The word at `0x006693e8` starts as 6.
     *
     * @ghidraAddress 0x006693e8
     */
    static int sCmdID;

private:
    GameRecorder *mRecorder; // +0x0c
};
