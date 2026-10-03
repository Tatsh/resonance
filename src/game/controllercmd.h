#pragma once

#include <iostream>

#include "msg/metcontrollerreading.h"
#include "sch/command.h"

class IBStream;
class OBStream;

/**
 * Scheduler command that replays one controller reading into the game world.
 *
 * It has Sch::Command as its one base. Its vtable at `0x007dc3c0` runs eight entries and overrides
 * every slot the base declares apart from Attachment::Destroy(). The allocation at `0x0018be70`
 * measures the object at 0x1c bytes, the 0x0c-byte base followed by one sixteen-byte reading.
 *
 * The static member sCmdID is attested by the RTTI, which records it as the first global of the
 * translation unit in the anonymous-namespace markers of ExitCmd and FuncCmd. Those two classes
 * share the unit with this one and with GrooveWorld, whose routines create all three.
 *
 * saveGuts() frames the reading between the tags `CM[` and `]CM`, one byte per character, and
 * restoreGuts() reads six bytes around the reading without checking them.
 *
 * The destructor at `0x001944e8` is implicitly declared. It stores the base table pointer and runs
 * Attachment's destructor, which is what the compiler generates.
 *
 * The member is private. Only this class's routines address it.
 */
class ControllerCmd : public Sch::Command {
public:
    /**
     * Start with the reading unset.
     *
     * Inline. NewCmd() expands it for restoreGuts() to fill.
     */
    ControllerCmd() {
    }

    /**
     * Record one controller reading to replay.
     *
     * Inline, with no address of its own. GrooveWorld::OnControllerReading() expands it when it
     * queues a reading.
     *
     * @param reading The reading.
     */
    explicit ControllerCmd(const MetControllerReading &reading) : mReading(reading) {
    }

    /**
     * Report the identifier this class streams itself under.
     *
     * @return sCmdID.
     * @ghidraAddress NTSC-U/C: 0x00194598
     * @ghidraAddress PAL: 0x0019a218
     */
    virtual int CmdID();

    /**
     * Hand the reading to the world of the running game.
     *
     * @ghidraAddress NTSC-U/C: 0x00194560
     * @ghidraAddress PAL: 0x0019a1e0
     */
    virtual void Execute();

    /**
     * Write `{ControllerCmd}` to a diagnostic stream.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x00194790
     * @ghidraAddress PAL: 0x0019a410
     */
    virtual void Print(std::ostream &stream);

    /**
     * Write the reading between the tags `CM[` and `]CM`.
     *
     * @param stream The stream to write to.
     * @ghidraAddress NTSC-U/C: 0x001945a8
     * @ghidraAddress PAL: 0x0019a228
     */
    virtual void saveGuts(OBStream &stream) const;

    /**
     * Read the reading back, discarding the three tag bytes on each side.
     *
     * @param stream The stream to read from.
     * @ghidraAddress NTSC-U/C: 0x001946b8
     * @ghidraAddress PAL: 0x0019a338
     */
    virtual void restoreGuts(IBStream &stream);

    /**
     * Produce a command with the reading unset, for the stream to load into.
     *
     * The factory the GrooveWorld unit's static initialiser registers. The expanded default
     * constructor writes only the base's words.
     *
     * @return The command.
     * @ghidraAddress NTSC-U/C: 0x0018be70
     * @ghidraAddress PAL: 0x00191918
     */
    static Sch::Command *NewCmd();

    /**
     * Identifier the class streams itself under. The word at `0x0067f238` starts as 2.
     *
     * @ghidraAddress NTSC-U/C: 0x0067f238
     * @ghidraAddress PAL: 0x006c0468
     */
    static int sCmdID;

private:
    MetControllerReading mReading; // +0x0c
};
