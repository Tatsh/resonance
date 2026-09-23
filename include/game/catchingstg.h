#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "game/catcher.h"
#include "game/phraseneutralizer.h"
#include "game/scoretrackgraph.h"
#include "game/trackdata.h"

/**
 * Gameplay stage for a track whose gems are caught rather than played.
 *
 * `11CatchingSTG` in the RTTI descriptor at `0x00902230`, with ScoreTrackGraph as its only base at
 * offset 0. Its table is at `0x007de4a0` and runs thirteen entries, the same as the base's, so the
 * class introduces no virtual. It is the one stage that overrides every slot, and the five defaults
 * the other three inherit are all replaced here. The object is 0x34 bytes.
 *
 * The constructor builds a PhraseNeutralizer and then one catcher, a SingleCatcher in game mode 1
 * and a MultiCatcher in every other mode. The catch window it hands the catcher is configuration
 * code 0x39c, in milliseconds, converted to MIDI ticks through the song clock's tempo map.
 *
 * Slot 12's body is not written. Its whole body is one call to an unidentified PhraseMgr routine
 * at `0x001ba3d8`.
 */
class CatchingSTG : public ScoreTrackGraph {
public:
    /**
     * Build the neutraliser and the catcher, and set the phrase manager's export lead.
     *
     * The export lead is 120 ticks for each track index plus 120.
     *
     * @param pTrackData The track description the base retains.
     * @ghidraAddress 0x0019fb80
     */
    CatchingSTG(const TrackData *pTrackData);

    /**
     * Stop the stage and release the catcher and the neutraliser.
     *
     * Slot 1.
     *
     * @ghidraAddress 0x001a0450
     */
    virtual ~CatchingSTG();

    /**
     * Start the stage.
     *
     * Slot 2. The routine starts the base and then schedules the catcher's commands through its
     * slot 4.
     *
     * @ghidraAddress 0x001a04d8
     */
    virtual void Slot2();

    /**
     * Stop the stage.
     *
     * Slot 3. The routine withdraws the catcher's commands through its slot 5 and then stops the
     * base.
     *
     * @ghidraAddress 0x001a0518
     */
    virtual void Slot3();

    /**
     * Wire the stage's objects to the sources that drive it.
     *
     * Slot 4.
     *
     * @param pPrimary The source the stage registers five of its objects with.
     * @param pOptional A further source, which receives the phrase manager and the catcher when it
     *                  is supplied.
     * @param pSecondary The source that receives the phrase manager and the catcher.
     * @ghidraAddress 0x0019fd88
     */
    virtual void Slot4(MsgSource *pPrimary, MsgSource *pOptional, MsgSource *pSecondary);

    /**
     * Register the mixer with one source.
     *
     * Slot 5. This class is the only one of the four that overrides the base's empty default.
     *
     * @param pSource The source to register with.
     * @ghidraAddress 0x001a0558
     */
    virtual void Slot5(MsgSource *pSource);

    /**
     * Attach the mixer to the synthesiser and give it its output sink.
     *
     * Slot 6.
     *
     * @param pOutput The sink the mixer sends to, stored in Mixer::mOutput.
     * @ghidraAddress 0x001a0588
     */
    virtual void Slot6(MsgSink *pOutput);

    /**
     * Register one sink with every source the stage provides.
     *
     * Slot 7.
     *
     * @param pSink The sink to register.
     * @ghidraAddress 0x001a05c8
     */
    virtual void Slot7(MsgSink *pSink);

    /**
     * Install the sink the phrase manager reports phrase changes to.
     *
     * Slot 8.
     *
     * @param pSink The sink to install, ignored when null.
     * @ghidraAddress 0x001a0650
     */
    virtual void Slot8(MsgSink *pSink);

    /**
     * Report whether the stage has nothing outstanding.
     *
     * Slot 9. The routine forwards to the catcher's slot 6.
     *
     * @return Non-zero when the catcher has nothing outstanding.
     * @ghidraAddress 0x001a06f0
     */
    virtual int Slot9();

    /**
     * Slot 10. The routine casts the catcher to SingleCatcher with `dynamic_cast` and forwards
     * both arguments to SingleCatcher::ResetOwners(). The cast is unguarded, so a MultiCatcher
     * yields a null receiver and the forwarded call dereferences it.
     *
     * The routine also calls the game-mode accessor on mApplication and discards the answer,
     * which is what remains of an assertion the release build compiled away.
     *
     * @param nTick The song position, forwarded unchanged.
     * @param pPlayer The player the phrases go to.
     * @ghidraAddress 0x001a0668
     */
    virtual void Slot10(int nTick, Player *pPlayer);

    /**
     * Slot 11. Returns 1 where the base returns zero.
     *
     * @return Always 1.
     * @ghidraAddress 0x001a0428
     */
    virtual int Slot11();

    /**
     * Slot 12. The whole body is one call on the phrase manager. The body is not written, for the
     * reason recorded in the class documentation.
     *
     * @ghidraAddress 0x001a0720
     */
    virtual void Slot12();

private:
    PhraseNeutralizer *mNeutralizer; // +0x2c
    Catcher *mCatcher;               // +0x30, a SingleCatcher in game mode 1
};
