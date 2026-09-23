#pragma once

#include "app/msgsink.h"
#include "gs/museparent.h"
#include "gs/museplayer.h"
#include "sch/cmdid.h"
#include "sch/tickclock.h"

/**
 * Player that sounds one note for a fixed length and then releases it.
 *
 * `10NotePlayer` in the RTTI descriptor, with MusePlayer as its one base. Its table at
 * `0x007e17b8` runs the type function, the destructor, and the three MusePlayer verbs. The object
 * is 0x20 bytes, which MuseSynth::StartNotePlayer() measures. That routine passes a NoteMsg's
 * bytes at `+0x09`, `+0x0a`, and `+0x08`, its word at `+0x0c`, the MuseParent half of the synth,
 * and the synth's clock.
 *
 * Start() sends the note-on and schedules the file-local command `Cmd` of `GsNotePlayer.cpp`
 * (`Q233_GLOBAL_$N$GsNotePlayer.cppdKuhgb3Cmd` in the RTTI) at the end of the note. That command
 * runs OnCommand(), which sends the note-off and reports the player finished to its parent. Stop()
 * sends the note-off early and withdraws the command.
 *
 * The note-on and note-off are StdMidiMsg objects built on the stack and delivered through
 * MsgSink::Handle() on mSink.
 */
class NotePlayer : public MusePlayer {
public:
    /**
     * Retain the note and shorten its length by two ticks, keeping at least one.
     *
     * @param nNote The MIDI note number.
     * @param nVelocity The note-on velocity.
     * @param nDuration The length of the note, in MIDI ticks.
     * @param nChannel The MIDI channel. Only the low four bits are kept.
     * @param pParent The owner the player reports to.
     * @param pClock The clock the note-off is scheduled against.
     * @ghidraAddress 0x001b4328
     */
    NotePlayer(unsigned char nNote,
               unsigned char nVelocity,
               int nDuration,
               unsigned char nChannel,
               MuseParent *pParent,
               Sch::TickClock *pClock);

    /**
     * Stop the note through this class's Stop().
     *
     * @ghidraAddress 0x001b4460
     */
    virtual ~NotePlayer();

    /**
     * Send the note-on to a sink and schedule the note-off.
     *
     * mParent is told to retain this player alone before the note-on is sent.
     *
     * @param pSink The sink both messages go to.
     * @ghidraAddress 0x001b3d58
     */
    virtual void Start(MsgSink *pSink);

    /**
     * Send the note-off now and withdraw the scheduled command, when the note is sounding.
     *
     * The note-off is a StdMidiMsg with status 0x80 ORed with mChannel, mNote, and velocity 0 at
     * the current song position. mSink is cleared whether or not the note was sounding.
     *
     * @ghidraAddress 0x001b3ee0
     */
    virtual void Stop();

    /**
     * @return Zero. A note never displaces its sibling players.
     * @ghidraAddress 0x001b41c0
     */
    virtual int Slot4();

    /**
     * Send the note-off at a song position and report the player finished to mParent.
     *
     * The file-local Cmd runs it. mSink is cleared between the two steps.
     *
     * @param nTick The song position the command was scheduled for.
     * @ghidraAddress 0x001b4040
     */
    void OnCommand(int nTick);

private:
    // Sends the note-on (status 0x90 ORed with mChannel, mNote, mVelocity) at a song position to
    // mSink.
    // 0x001b3fb8
    void PostStdMidiMsg(int nTick);

    unsigned char mNote;     // +0x08
    unsigned char mVelocity; // +0x09
    unsigned char mChannel;  // +0x0a
    int mDuration;           // +0x0c
    MsgSink *mSink;          // +0x10, set by Start() and cleared when the note ends
    MuseParent *mParent;     // +0x14
    Sch::TickClock *mClock;  // +0x18
    CmdID mCommand;          // +0x1c
};
