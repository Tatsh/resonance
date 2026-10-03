#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/autoriffer.h"
#include "game/axenewgemmaker.h"
#include "game/axeoldgemmaker.h"
#include "game/axephrasemaker.h"
#include "game/axiscontrol.h"
#include "game/gsperiodical.h"
#include "game/pitchpicker.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"
#include "gs/musesynth.h"
#include "synth/synthsustainer.h"

/**
 * Gameplay stage for a guitar track.
 *
 * Its RTTI descriptor is at `0x008efe70`. It has ScoreTrackGraph as its only base at offset 0. Its
 * table is at `0x007ddf88` and runs thirteen entries, the same as the base's. The class introduces
 * no virtual. It overrides eight slots and inherits slots 5, 9, 10, 11, and 12 from the
 * base. The object is 0x54 bytes.
 *
 * The constructor builds nine objects. The GsPeriodical goes through the plain allocator and the
 * other eight through the tagged allocator. It also calls MuseSynth::CreateSustainer() on the
 * base's synthesiser before it builds anything of its own.
 *
 * The member at `+0x34` is the one the constructor never writes. It is declared so that the two
 * members around it retain their offsets, and no reader for it was found.
 *
 * The names of three of its members (AutoRiffer, AxisControl, and AxePhraseMaker) come from the
 * descriptor that the slot 0 accessor of each one's table guards on.
 */
class AxingSTG : public ScoreTrackGraph {
public:
    /**
     * @param pTrackData The track description the base retains.
     * @ghidraAddress NTSC-U/C: 0x0019daf0
     * @ghidraAddress PAL: 0x001a3858
     */
    AxingSTG(TrackData *pTrackData);

    /**
     * Stop the stage and release everything the constructor built.
     *
     * Slot 1. The routine stops the stage through a direct call to this class's own Stop(), then
     * releases the periodical post and the eight objects it built, in an order that is not the
     * order of their offsets.
     *
     * @ghidraAddress NTSC-U/C: 0x0019de08
     * @ghidraAddress PAL: 0x001a3b70
     */
    virtual ~AxingSTG();

    /**
     * Start the stage.
     *
     * Slot 2. In play mode 2 the routine sends controller 0x52 with value 0x7f on the track's MIDI
     * channel, then starts the base and queues the periodical post.
     *
     * @ghidraAddress NTSC-U/C: 0x0019e720
     * @ghidraAddress PAL: 0x001a4488
     */
    virtual void Start();

    /**
     * Stop the stage.
     *
     * Slot 3. In play mode 2 the routine sends controller 0x52 with value zero on the track's MIDI
     * channel, then withdraws the periodical post and stops the base.
     *
     * @ghidraAddress NTSC-U/C: 0x0019e798
     * @ghidraAddress PAL: 0x001a4500
     */
    virtual void Stop();

    /**
     * Wire the stage's objects to the sources that drive it.
     *
     * Slot 4.
     *
     * @param pPrimary The source the stage registers eight of its objects with.
     * @param pOptional A further source, which receives the phrase manager when it is supplied.
     * @param pSecondary The source that receives the phrase manager and the phrase maker.
     * @ghidraAddress NTSC-U/C: 0x0019df58
     * @ghidraAddress PAL: 0x001a3cc0
     */
    virtual void ConnectSources(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary);

    /**
     * Attach the mixer to the base's synthesiser and give it its output sink.
     *
     * Slot 6.
     *
     * @param pOutput The sink the mixer sends to, stored in Mixer::mOutput.
     * @ghidraAddress NTSC-U/C: 0x0019e810
     * @ghidraAddress PAL: 0x001a4578
     */
    virtual void SetMixerOutput(MsgSink *pOutput);

    /**
     * Register one sink with every source the stage provides.
     *
     * Slot 7.
     *
     * @param pSink The sink to register.
     * @ghidraAddress NTSC-U/C: 0x0019e850
     * @ghidraAddress PAL: 0x001a45b8
     */
    virtual void AddSinkToSources(MsgSink *pSink);

    /**
     * Install the sink the phrase manager reports phrase changes to.
     *
     * Slot 8.
     *
     * @param pSink The sink to install, ignored when null.
     * @ghidraAddress NTSC-U/C: 0x0019e928
     * @ghidraAddress PAL: 0x001a4690
     */
    virtual void SetNetSink(MsgSink *pSink);

private:
    AutoRiffer *mAutoRiffer;      // +0x2c
    PitchPicker *mPitchPicker;    // +0x30
    int mReserved;                // +0x34, never read or written
    AxisControl *mAxisControl;    // +0x38
    AxeNewGemMaker *mNewGemMaker; // +0x3c
    AxeOldGemMaker *mOldGemMaker; // +0x40
    MuseSynth *mAxeSynth;         // +0x44, distinct from the base's synthesiser at +0x14
    SynthSustainer *mSustainer;   // +0x48
    AxePhraseMaker *mPhraseMaker; // +0x4c
    GsPeriodical *mPeriodical;    // +0x50
};
