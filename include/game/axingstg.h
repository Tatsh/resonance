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
 * `8AxingSTG` in the RTTI descriptor at `0x008efe70`, with ScoreTrackGraph as its only base at
 * offset 0. Its table is at `0x007ddf88` and runs thirteen entries, the same as the base's, so the
 * class introduces no virtual. It overrides eight slots and inherits slots 5, 9, 10, 11, and 12
 * from the base. The object is 0x54 bytes.
 *
 * The constructor builds nine objects. The GsPeriodical goes through the plain allocator and the
 * other eight through the tagged allocator. It also calls MuseSynth::CreateSustainer() on the
 * base's synthesiser before it builds anything of its own.
 *
 * The member at `+0x34` is the one the constructor never writes. It is declared so that the two
 * members around it retain their offsets, and no reader for it was found.
 *
 * Three of this class's members were titled for renderer classes by an earlier pass. AutoRiffer
 * was `RndSpotShadowMeshPass`, AxisControl was `RndSpotShadowCam`, and AxePhraseMaker was
 * `RndSpotShadowMap`. No descriptor among the 574 in the image bears any of the three titles, and
 * each real name comes from the descriptor its table's slot 0 accessor guards on.
 */
class AxingSTG : public ScoreTrackGraph {
public:
    /**
     * @param pTrackData The track description the base retains.
     * @ghidraAddress 0x0019daf0
     */
    AxingSTG(TrackData *pTrackData);

    /**
     * Stop the stage and release everything the constructor built.
     *
     * Slot 1. The routine stops the stage through a direct call to this class's own Slot3(), then
     * releases the periodical post and the eight objects it built, in an order that is not the
     * order of their offsets.
     *
     * @ghidraAddress 0x0019de08
     */
    virtual ~AxingSTG();

    /**
     * Start the stage.
     *
     * Slot 2. In play mode 2 the routine sends controller 0x52 with value 0x7f on the track's MIDI
     * channel, then starts the base and queues the periodical post.
     *
     * @ghidraAddress 0x0019e720
     */
    virtual void Slot2();

    /**
     * Stop the stage.
     *
     * Slot 3. In play mode 2 the routine sends controller 0x52 with value zero on the track's MIDI
     * channel, then withdraws the periodical post and stops the base.
     *
     * @ghidraAddress 0x0019e798
     */
    virtual void Slot3();

    /**
     * Wire the stage's objects to the sources that drive it.
     *
     * Slot 4.
     *
     * @param pPrimary The source the stage registers eight of its objects with.
     * @param pOptional A further source, which receives the phrase manager when it is supplied.
     * @param pSecondary The source that receives the phrase manager and the phrase maker.
     * @ghidraAddress 0x0019df58
     */
    virtual void Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary);

    /**
     * Attach the mixer to the base's synthesiser and give it its output sink.
     *
     * Slot 6.
     *
     * @param pOutput The sink the mixer sends to, stored in Mixer::mOutput.
     * @ghidraAddress 0x0019e810
     */
    virtual void Slot6(MsgSink *pOutput);

    /**
     * Register one sink with every source the stage provides.
     *
     * Slot 7.
     *
     * @param pSink The sink to register.
     * @ghidraAddress 0x0019e850
     */
    virtual void Slot7(MsgSink *pSink);

    /**
     * Install the sink the phrase manager reports phrase changes to.
     *
     * Slot 8.
     *
     * @param pSink The sink to install, ignored when null.
     * @ghidraAddress 0x0019e928
     */
    virtual void Slot8(MsgSink *pSink);

private:
    AutoRiffer *mAutoRiffer;      // +0x2c
    PitchPicker *mPitchPicker;    // +0x30
    int mUnknown34;               // +0x34, the constructor does not write it
    AxisControl *mAxisControl;    // +0x38
    AxeNewGemMaker *mNewGemMaker; // +0x3c
    AxeOldGemMaker *mOldGemMaker; // +0x40
    MuseSynth *mAxeSynth;         // +0x44, distinct from the base's synthesiser at +0x14
    SynthSustainer *mSustainer;   // +0x48
    AxePhraseMaker *mPhraseMaker; // +0x4c
    GsPeriodical *mPeriodical;    // +0x50
};
