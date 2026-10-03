#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"

class Message;
class NoteMsg;
class StdMidiMsg;

/**
 * Filter that stops a track's notes while it is disabled and passes everything else.
 *
 * It has MsgSource at offset 0 and MsgSink at `+0x14`. Its tables are at `0x007df530` and
 * `0x007df508`, the second adjusting `this` by `-20`. The unit spans `0x001a6a58` through
 * `0x001a6fb8`. BGTrackGraph's constructor creates the one instance each background track has, with
 * the constructor expanded inline.
 *
 * While mEnabled is clear, HandleMessage() drops every NoteMsg and every StdMidiMsg whose status
 * is a note-off or a note-on, and forwards the rest.
 *
 * The unreferenced forwarder at `0x001a6ed8` in this unit, byte-identical to
 * MsgJoiner::HandleMessage() at `0x00195b70`, has its unwind record at `0x006852d8` as its only
 * reference and is recorded here rather than declared.
 */
class MidiDisabler : public MsgSource, public MsgSink {
public:
    /**
     * @param bEnabled Non-zero to start passing notes.
     * @ghidraAddress NTSC-U/C: 0x001a6e10
     * @ghidraAddress PAL: 0x001acb78
     */
    explicit MidiDisabler(int bEnabled);

    /**
     * @ghidraAddress NTSC-U/C: 0x001a6ac0
     * @ghidraAddress PAL: 0x001ac828
     */
    virtual ~MidiDisabler();

    /**
     * Forward the message unless it is a note the filter stops.
     *
     * StdMidiMsg and NoteMsg go through their OnMsg() overloads, which are expanded inline here.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x001a6f08
     * @ghidraAddress PAL: 0x001acc70
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Start passing notes.
     *
     * The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x001a6ef8
     * @ghidraAddress PAL: 0x001acc60
     */
    void Enable();

    /**
     * Stop passing notes, and silence the notes already sounding with an AllNotesOffMsg.
     *
     * The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x001a6a58
     * @ghidraAddress PAL: 0x001ac7c0
     */
    void Disable();

private:
    // NTSC-U/C: 0x001a6e60, PAL: 0x001acbc8
    // Forwards the message unless notes are stopped and its status is a note-off or a
    // note-on.
    void OnMsg(StdMidiMsg &msg);

    // NTSC-U/C: 0x001a6eb0, PAL: 0x001acc18
    // Forwards the message unless notes are stopped.
    void OnMsg(NoteMsg &msg);

    int mEnabled; // +0x18
};
