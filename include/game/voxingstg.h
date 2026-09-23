#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/axenewgemmaker.h"
#include "game/axeoldgemmaker.h"
#include "game/jameffectsmgr.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"
#include "gs/voxer.h"

/**
 * Gameplay stage for a vocal track.
 *
 * `9VoxingSTG` in the RTTI descriptor at `0x008efe80`, with ScoreTrackGraph as its only base at
 * offset 0. Its table is at `0x007e6460` and runs thirteen entries, the same as the base's, so the
 * class introduces no virtual. It overrides eight slots and inherits slots 5, 9, 10, 11, and 12
 * from the base. The object is 0x3c bytes.
 *
 * The constructor builds the producer and both gem makers unconditionally, and the jam effects
 * manager only in play mode 2. The two members it clears first are the producer and the jam
 * effects manager, and the destructor guards the release of each.
 *
 * The constructor's body is not written, for the reason recorded on AxingSTG. Slot 2 and slot 3
 * are declared and not written, for the reason recorded on PitchingSTG.
 */
class VoxingSTG : public ScoreTrackGraph {
public:
    /**
     * @param pTrackData The track description the base retains.
     * @ghidraAddress 0x001da050
     */
    VoxingSTG(const TrackData *pTrackData);

    /**
     * Stop the stage and release the four objects the constructor built.
     *
     * Slot 1. The release order is the two gem makers, the producer, and then the jam effects
     * manager, which is neither the order of their offsets nor the order they were built in.
     *
     * @ghidraAddress 0x001da730
     */
    virtual ~VoxingSTG();

    /**
     * Start the stage.
     *
     * Slot 2. The routine starts the base and then posts the producer's tick task. The body is
     * not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x001da7f8
     */
    virtual void Slot2();

    /**
     * Stop the stage.
     *
     * Slot 3. The routine withdraws the producer's tick task and then stops the base. The body is
     * not written, for the reason recorded in the class documentation.
     *
     * @ghidraAddress 0x001da830
     */
    virtual void Slot3();

    /**
     * Wire the stage's objects to the sources that drive it.
     *
     * Slot 4.
     *
     * @param pPrimary The source the stage registers five or six of its objects with.
     * @param pOptional A further source, which receives the phrase manager when it is supplied.
     * @param pSecondary The source that receives the phrase manager and the producer.
     * @ghidraAddress 0x001da230
     */
    virtual void Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary);

    /**
     * Install a word in the mixer and attach the mixer to the synthesiser.
     *
     * Slot 6.
     *
     * @param nValue The word to install.
     * @ghidraAddress 0x001da868
     */
    virtual void Slot6(int nValue);

    /**
     * Register one sink with every source the stage provides.
     *
     * Slot 7.
     *
     * @param pSink The sink to register.
     * @ghidraAddress 0x001da8a8
     */
    virtual void Slot7(MsgSink *pSink);

    /**
     * Install a word in the phrase manager.
     *
     * Slot 8.
     *
     * @param nValue The word to install, ignored when zero.
     * @ghidraAddress 0x001da978
     */
    virtual void Slot8(int nValue);

private:
    Voxer *mVoxer;                // +0x2c
    JamEffectsMgr *mJamEffects;   // +0x30, null outside play mode 2
    AxeNewGemMaker *mNewGemMaker; // +0x34
    AxeOldGemMaker *mOldGemMaker; // +0x38
};
