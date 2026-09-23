#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/jameffectsmgr.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"
#include "gs/notepitcher.h"
#include "gs/pitcher.h"
#include "gs/scratcher.h"

/**
 * Gameplay stage for a track whose input is a pitch.
 *
 * `11PitchingSTG` in the RTTI descriptor at `0x00902240`, with ScoreTrackGraph as its only base at
 * offset 0. Its table is at `0x007e3648` and runs thirteen entries, the same as the base's, so the
 * class introduces no virtual. It overrides eight slots and inherits slots 5, 9, 10, 11, and 12
 * from the base. The object is 0x34 bytes.
 *
 * The constructor picks the producer from the track's kind word, a NotePitcher for 2 and a
 * Scratcher for 3, and it builds neither for any other value. It builds the jam effects manager
 * only in play mode 2. Both members therefore start as null pointers and the destructor guards
 * each release.
 *
 * The constructor's body is not written, for the reason recorded on AxingSTG.
 *
 * Slot 2 and slot 3 are declared and not written. Each calls the base and one routine on the
 * producer's TickTask subobject, `0x0013a860` to post the task with the position kMBTInfinity and
 * `0x0013ae10` to withdraw it and reset the handle at TickTask `+0x0c` to -2. Neither routine is
 * declared on TickTask, and `app/ticktask.h` is outside the assignment this class was recovered
 * under.
 */
class PitchingSTG : public ScoreTrackGraph {
public:
    /**
     * @param pTrackData The track description the base retains.
     * @ghidraAddress 0x001c45b0
     */
    PitchingSTG(const TrackData *pTrackData);

    /**
     * Stop the stage and release the producer and the jam effects manager.
     *
     * Slot 1.
     *
     * @ghidraAddress 0x001c4c68
     */
    virtual ~PitchingSTG();

    /**
     * Start the stage.
     *
     * Slot 2. The routine starts the base and then posts the producer's tick task. The body is
     * not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x001c4cf0
     */
    virtual void Slot2();

    /**
     * Stop the stage.
     *
     * Slot 3. The routine withdraws the producer's tick task and then stops the base. The body is
     * not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x001c4d28
     */
    virtual void Slot3();

    /**
     * Wire the stage's objects to the sources that drive it.
     *
     * Slot 4.
     *
     * @param pPrimary The source the stage registers four or five of its objects with.
     * @param pOptional A further source, which receives the phrase manager when it is supplied.
     * @param pSecondary The source that receives the phrase manager and the producer.
     * @ghidraAddress 0x001c4798
     */
    virtual void Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary);

    /**
     * Attach the mixer to the synthesiser and give it its output sink.
     *
     * Slot 6.
     *
     * @param pOutput The sink the mixer sends to, stored in Mixer::mOutput.
     * @ghidraAddress 0x001c4d60
     */
    virtual void Slot6(MsgSink *pOutput);

    /**
     * Register one sink with every source the stage provides.
     *
     * Slot 7.
     *
     * @param pSink The sink to register.
     * @ghidraAddress 0x001c4da0
     */
    virtual void Slot7(MsgSink *pSink);

    /**
     * Install the sink the phrase manager reports phrase changes to.
     *
     * Slot 8.
     *
     * @param pSink The sink to install, ignored when null.
     * @ghidraAddress 0x001c4e30
     */
    virtual void Slot8(MsgSink *pSink);

private:
    Pitcher *mPitcher;          // +0x2c, a NotePitcher or a Scratcher, null for any other kind
    JamEffectsMgr *mJamEffects; // +0x30, null outside play mode 2
};
