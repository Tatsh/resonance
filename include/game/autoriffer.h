#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/axephrasemaker.h"
#include "game/player.h"
#include "game/quantizer.h"
#include "game/riff.h"
#include "game/trackdata.h"
#include "msg/erasemsg.h"
#include "msg/message.h"
#include "msg/pitchriffmsg.h"
#include "msg/stopriffmsg.h"
#include "msg/trackselectmsg.h"
#include "sch/cmdid.h"
#include "sch/tickclock.h"
#include "synth/musesynth.h"

/**
 * Producer of the automatic riff a guitar track plays when it is not being played.
 *
 * `10AutoRiffer` in the RTTI descriptor at `0x008eef38`, deriving from MsgSink at offset 0, so the
 * base vptr lands at `+0x00` and this class's own members start at `+0x04`. Its table is at
 * `0x007dd488` with four entries. The object is 0x4c bytes, which AxingSTG's tagged allocation
 * measures.
 *
 * An earlier pass titled this class's constructor `RndSpotShadowMeshPass__Ctor`. No descriptor
 * among the 574 in the image bears that title. Slot 0 of the table at `0x007dd488` addresses the
 * accessor at `0x0019a490`, which guards on the descriptor at `0x008eef38`, and that is what
 * settles the name.
 *
 * The source at `+0x30` is a data member rather than a base. The constructor constructs it at that
 * offset, and every caller calls MsgSource::AddSink() on it directly rather than through a vptr.
 *
 * HandleMessage() dispatches five identities. A PitchRiffMsg goes to OnPitchRiff(), an EraseMsg
 * to OnErase(), a StopRiffMsg to OnStopRiff(), and a GameOverMsg to StopRiff() at position 0. A
 * TrackSelectMsg runs the inline copy of OnTrackSelect(). Most handlers send an AllNotesOffMsg,
 * an AxeButtonMsg, or a MultiMuseMsg built on the stack, and those classes declare their payload
 * private with no constructor that takes it, so their bodies are not written. Each is described
 * where it is declared.
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
     * @ghidraAddress 0x00199040
     */
    AutoRiffer(Sch::TickClock *pClock, Quantizer *pQuantizer, const TrackData *pTrackData);

    /**
     * @ghidraAddress 0x0019a3d0
     */
    virtual ~AutoRiffer();

    /**
     * Act on a message.
     *
     * The body is not written, because its TrackSelectMsg branch reads the message's private
     * position at `+0x0c`.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x00199910
     */
    virtual void HandleMessage(Message *pMsg);

    /**
     * Repeat the current riff at a song position, or release the buttons.
     *
     * The file-local Cmd runs it. When the routine at `0x0019da58` on mPhraseMaker reports 1 for
     * the bar of the position, it plays the current riff again through PlayRiff(). Otherwise it
     * sends an AxeButtonMsg for mPlayer that releases every button. The body is not written, for
     * the reason recorded in the class documentation.
     *
     * @param nTick The song position the command was scheduled for.
     * @ghidraAddress 0x00199688
     */
    void OnCommand(int nTick);

    /**
     * Add a sink to mSource.
     *
     * The image has no caller. AxingSTG calls MsgSource::AddSink() on mSource directly.
     *
     * @param pSink The sink.
     * @ghidraAddress 0x0019a508
     */
    void AddSink(MsgSink *pSink);

private:
    // Starts the riff of the message's level at its quantised position for this track's player.
    // A position inside a phrase bar plays SND_INACTIVE instead. Not written, for the reason
    // recorded in the class documentation.
    // 0x00199160
    void OnPitchRiff(PitchRiffMsg *pMsg);

    // Clears the held flag of the message's level and switches to another held level's riff, or
    // stops when none is held. Not written, for the same reason.
    // 0x001992e0
    void OnStopRiff(StopRiffMsg *pMsg);

    // Stops the riff and hands the erase to mPhraseMaker through the routine at 0x0019bf80 when
    // the bar is a phrase bar. Not written, for the same reason.
    // 0x00199480
    void OnErase(EraseMsg *pMsg);

    // When a riff is playing, clears every held flag, sends an AllNotesOffMsg, withdraws
    // mCommand, and releases every button with an AxeButtonMsg. Not written, for the same reason.
    // 0x00199590
    void StopRiff(int nTick);

    // Sends an AllNotesOffMsg to mSynth and the current riff as a MultiMuseMsg, then schedules
    // the file-local Cmd at the end of the riff after the rounded position. Not written, for the
    // same reason.
    // 0x00199758
    void PlayRiff(int nTick);

    // The out-of-line copy of the TrackSelectMsg branch HandleMessage() expands inline. Not
    // written, because it reads the message's private position at `+0x0c`.
    // 0x0019a898
    void OnTrackSelect(TrackSelectMsg *pMsg);

    int mTrack;                  // +0x04, copied from TrackData::mUnknown04
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
     * The constructor clears this member and the one below, and AxingSTG::Slot4() writes both from
     * outside the class, which is what records them public. A friend declaration on AxingSTG fits
     * the image equally well.
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
