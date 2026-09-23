#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"

class Message;
class NoteMsg;
class StdMidiMsg;

/**
 * Filter that stops a track's notes while it is disabled and passes everything else.
 *
 * `12MidiDisabler` in the RTTI descriptor, with MsgSource at offset 0 and MsgSink at `+0x14`. Its
 * tables are at `0x007df530` and `0x007df508`, the second adjusting `this` by `-20`. The unit spans
 * `0x001a6a58` through `0x001a6fb8`. BGTrackGraph's constructor creates the one instance each
 * background track has, with the constructor expanded inline.
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
     * @ghidraAddress 0x001a6e10
     */
    explicit MidiDisabler(int bEnabled);

    /** @ghidraAddress 0x001a6ac0 */
    virtual ~MidiDisabler();

    /**
     * Forward the message unless it is a note the filter stops.
     *
     * StdMidiMsg and NoteMsg go through PassStdMidi() and PassNote(), which are expanded inline
     * here.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x001a6f08
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Start passing notes.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x001a6ef8
     */
    void Enable();

    /**
     * Stop passing notes, and silence the notes already sounding with an AllNotesOffMsg.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x001a6a58
     */
    void Disable();

private:
    // 0x001a6e60
    // Forwards the message unless notes are stopped and its status is a note-off or a
    // note-on.
    void PassStdMidi(StdMidiMsg *pMsg);

    // 0x001a6eb0
    // Forwards the message unless notes are stopped.
    void PassNote(NoteMsg *pMsg);

    int mEnabled; // +0x18
};
