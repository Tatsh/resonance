#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/axephrasemaker.h"
#include "game/player.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "msg/message.h"
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
 * The source at `+0x30` is a data member rather than a base, which the constructor proves by
 * constructing it at that offset and which every caller confirms by calling MsgSource::AddSink()
 * on it directly rather than through a vptr.
 *
 * The class is not reconstructed. Only the surface AxingSTG uses is declared, so that it compiles
 * against the real type.
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
     * @param pMsg The message.
     * @ghidraAddress 0x00199910
     */
    virtual void HandleMessage(Message *pMsg);

private:
    int mUnknown04;              // +0x04, copied from TrackData::mUnknown04
    Sch::TickClock *mClock;      // +0x08
    Quantizer *mQuantizer;       // +0x0c
    int mUnknown10;              // +0x10
    int mUnknown14[4];           // +0x14, a sixteen-byte run the constructor clears
    const TrackData *mTrackData; // +0x24
    int mUnknown28;              // +0x28, starts -2

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
    Player *mUnknown48; // +0x48, the file-scope NullPlayer until one is assigned
};
