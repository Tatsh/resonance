#pragma once

#include <iostream>

#include "sch/command.h"

class GameRecorder;
class OBStream;

/**
 * Scheduler command that ends a recording.
 *
 * `EndRecordingCmd` is one of the five ordinary Sch::Command subclasses. Its table at `0x007cdcb0`
 * has nine entries: it overrides saveGuts() with an empty body of its own, inherits
 * Sch::Command::restoreGuts() at `0x00539f28`, and adds one virtual of its own at slot 8, which is
 * empty. GameRecorder::ScheduleEnd() allocates the 0x10-byte object with the untagged allocator and
 * expands the constructor. The static initialiser of the unit registers New() under identifier 6
 * with the factory registrar.
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
     * @ghidraAddress NTSC-U/C: 0x0010c8d0
     * @ghidraAddress PAL: 0x0010caa0
     */
    static Sch::Command *New();

    /**
     * Report sCmdID.
     *
     * @return The class's command identifier.
     * @ghidraAddress NTSC-U/C: 0x0010efc8
     * @ghidraAddress PAL: 0x0010f428
     */
    virtual int CmdID();

    /**
     * Run GameRecorder::EndRecording() on the recorder.
     *
     * @ghidraAddress NTSC-U/C: 0x0010efa8
     * @ghidraAddress PAL: 0x0010f408
     */
    virtual void Execute();

    /**
     * Write `{EndRecordingCmd}`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x0010efe8
     * @ghidraAddress PAL: 0x0010f448
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write nothing.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x0010efd8
     * @ghidraAddress PAL: 0x0010f438
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Do nothing. Slot 8, the one virtual this class adds.
     *
     * The body is empty and no caller is recovered.
     *
     * @ghidraAddress NTSC-U/C: 0x0010efe0
     * @ghidraAddress PAL: 0x0010f440
     */
    virtual void UnusedHook();

    /**
     * Identifier the class streams itself under. The word at `0x006693e8` starts as 6.
     *
     * @ghidraAddress NTSC-U/C: 0x006693e8
     * @ghidraAddress PAL: 0x006a9f78
     */
    static int sCmdID;

private:
    GameRecorder *mRecorder; // +0x0c
};
