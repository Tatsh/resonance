#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/axephrasemaker.h"
#include "game/player.h"
#include "game/quantizer.h"
#include "game/riff.h"
#include "game/trackdata.h"
#include "gs/musesynth.h"
#include "msg/erasemsg.h"
#include "msg/message.h"
#include "msg/pitchriffmsg.h"
#include "msg/stopriffmsg.h"
#include "msg/trackselectmsg.h"
#include "sch/cmdid.h"
#include "sch/tickclock.h"

/**
 * Producer of the automatic riff a guitar track plays when it is not being played.
 *
 * Its RTTI descriptor is at `0x008eef38`. It derives from MsgSink at offset 0. The base vptr lands
 * at `+0x00`, and this class's members start at `+0x04`. Its table is at `0x007dd488` with four
 * entries. The object is 0x4c bytes, measured by AxingSTG's tagged allocation.
 *
 * Slot 0 of the table at `0x007dd488` addresses the accessor at `0x0019a490`. The accessor guards
 * on the descriptor at `0x008eef38`, and the descriptor settles the class name.
 *
 * The source at `+0x30` is a data member rather than a base. The constructor constructs it at that
 * offset, and every caller calls MsgSource::AddSink() on it directly rather than through a vptr.
 *
 * DispatchPriv() dispatches five identities. A PitchRiffMsg goes to OnPitchRiff(), an EraseMsg
 * to OnErase(), a StopRiffMsg to OnStopRiff(), and a GameOverMsg to StopRiff() at position 0. A
 * TrackSelectMsg runs the inline copy of OnTrackSelect().
 *
 * The file-local command class `Cmd`, in the anonymous namespace of `GsAutoRiffer.cpp`, runs
 * OnCommand() at the position PlayRiff() schedules.
 */
class AutoRiffer : public MsgSink {
public:
    /**
     * @param pClock The clock the riffer schedules against.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description.
     * @ghidraAddress NTSC-U/C: 0x00199040
     * @ghidraAddress PAL: 0x0019eda8
     */
    AutoRiffer(Sch::TickClock *pClock, Quantizer *pQuantizer, const TrackData *pTrackData);

    /**
     * @ghidraAddress NTSC-U/C: 0x0019a3d0
     * @ghidraAddress PAL: 0x001a0138
     */
    virtual ~AutoRiffer();

    /**
     * Act on a message.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x00199910
     * @ghidraAddress PAL: 0x0019f678
     */
    virtual void DispatchPriv(Message *pMsg);

    /**
     * Repeat the current riff at a song position, or release the buttons.
     *
     * The file-local Cmd runs it. When AxePhraseMaker::IsBarPlayable() reports 1 for the bar of
     * the position, it plays the current riff again through PlayRiff(). Otherwise it sends an
     * AxeButtonMsg for mPlayer that releases every button.
     *
     * @param nTick The song position the command was scheduled for.
     * @ghidraAddress NTSC-U/C: 0x00199688
     * @ghidraAddress PAL: 0x0019f3f0
     */
    void OnCommand(int nTick);

    /**
     * Add a sink to mSource.
     *
     * The image has no caller. AxingSTG calls MsgSource::AddSink() on mSource directly.
     *
     * @param pSink The sink.
     * @ghidraAddress NTSC-U/C: 0x0019a508
     * @ghidraAddress PAL: 0x001a0270
     */
    void AddSink(MsgSink *pSink);

private:
    // Starts the riff of the message's level at its quantised position for this track's player.
    // A bar AxePhraseMaker::IsBarPlayable() rejects plays SND_INACTIVE instead.
    // NTSC-U/C: 0x00199160, PAL: 0x0019eec8
    void OnPitchRiff(PitchRiffMsg *pMsg);

    // Clears the held flag of the message's level and switches to the lowest held level's riff at
    // the quantised position. With no level held it sends an AllNotesOffMsg, withdraws mCommand,
    // and releases the button, without the flag reset StopRiff() performs.
    // NTSC-U/C: 0x001992e0, PAL: 0x0019f048
    void OnStopRiff(StopRiffMsg *pMsg);

    // Stops the riff and hands the erase to AxePhraseMaker::Erase() when the bar is playable.
    // NTSC-U/C: 0x00199480, PAL: 0x0019f1e8
    void OnErase(EraseMsg *pMsg);

    // When a riff is playing, clears every held flag, sends an AllNotesOffMsg, withdraws
    // mCommand, and releases every button with an AxeButtonMsg.
    // NTSC-U/C: 0x00199590, PAL: 0x0019f2f8
    void StopRiff(int nTick);

    // Sends an AllNotesOffMsg to mSynth and the current riff as a MultiMuseMsg, then schedules
    // the file-local Cmd at the end of the riff after the rounded position.
    // NTSC-U/C: 0x00199758, PAL: 0x0019f4c0
    void PlayRiff(int nTick);

    // The out-of-line copy of the GameOverMsg branch DispatchPriv() expands inline. It stops the
    // riff at position zero and reads nothing from the message.
    // NTSC-U/C: 0x0019a920, PAL: 0x001a0688
    void OnGameOver();

    // The out-of-line copy of the TrackSelectMsg branch DispatchPriv() expands inline.
    // NTSC-U/C: 0x0019a898, PAL: 0x001a0600
    void OnTrackSelect(TrackSelectMsg *pMsg);

    int mTrack;                  // +0x04, copied from TrackData::mIndex
    Quantizer *mQuantizer;       // +0x08
    const TrackData *mTrackData; // +0x0c
    Riff *mCurrentRiff;          // +0x10
    int mLevelHeld[4];           // +0x14, one flag per difficulty level, cleared with memset
    Sch::TickClock *mClock;      // +0x24
    CmdID mCommand;              // +0x28, the handle the file-local Cmd is queued under

public:
    /**
     * Synthesiser AxingSTG installs while it wires the stage up.
     *
     * The constructor clears this member and the one below, and AxingSTG::ConnectInputs() writes
     * both from outside the class. Those outside writes record them public. A friend declaration on
     * AxingSTG fits the image equally well.
     *
     * +0x2c
     */
    MuseSynth *mSynth;

    /** Sinks the riffer publishes to. +0x30 */
    MsgSource mSource;

    /** Phrase maker AxingSTG installs. +0x44 */
    AxePhraseMaker *mPhraseMaker;

private:
    Player *mPlayer; // +0x48, the file-scope NullPlayer until a TrackSelectMsg assigns one
};
