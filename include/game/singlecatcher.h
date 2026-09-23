#pragma once

#include "game/catcher.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "sch/tick.h"
#include "sch/tickclock.h"

/**
 * Catcher that scores one bar at a time.
 *
 * `13SingleCatcher` in the RTTI descriptor at `0x008f0be0`, with Catcher as its only base at offset
 * 0. Its primary table is at `0x007e0bb0` with eleven entries and its MsgSource subobject table at
 * `0x007e0b88` with four. The table is the same length as the base's, so the class introduces no
 * virtual and overrides only the destructor and the base's two pure slots. The object is 0x84
 * bytes against the base's 0x80, so it adds one word, and that word is not recovered.
 *
 * The class is not reconstructed. Only the surface CatchingSTG and the table walk establish is
 * declared. Its constructor is at `0x001b19b0` and its destructor at `0x001b0d30`.
 */
class SingleCatcher : public Catcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description.
     * @param pClock The clock the scheduled commands run on.
     * @param tick The scheduler time the base retains.
     * @ghidraAddress 0x001b19b0
     */
    SingleCatcher(PhraseMgr *pPhraseMgr,
                  Quantizer *pQuantizer,
                  const TrackData *pTrackData,
                  Sch::TickClock *pClock,
                  Sch::Tick tick);

    /**
     * @ghidraAddress 0x001b0d30
     */
    virtual ~SingleCatcher();

    /**
     * Score one bar.
     *
     * Slot 9.
     *
     * @param nFirst The bar.
     * @param nSecond The second word.
     * @param nThird Zero selects a different query, as in MultiCatcher.
     * @ghidraAddress 0x001ad630
     */
    virtual void Slot9(int nFirst, int nSecond, int nThird);

    /**
     * Report the caught power bar to this catcher's player.
     *
     * Slot 10. The routine queries the phrase manager and, for an answer other than -1, sends a
     * CaughtPowerbarMsg carrying that answer and the player, then delivers the same message to the
     * player directly through MsgSink::Handle().
     *
     * @ghidraAddress 0x001ad840
     */
    virtual void Slot10();

    /**
     * Score two words against this catcher.
     *
     * The routine is the target of ScoreTrackGraph's slot 10, which CatchingSTG reaches by casting
     * its catcher to this class with `dynamic_cast`. Both arguments pass through unchanged and the
     * answer is returned to that slot's caller. The title is inferred from that one call site; no
     * literal in the image attests the original.
     *
     * @param nFirst The first word.
     * @param nSecond The second word.
     * @return The answer ScoreTrackGraph::Slot10() reports.
     * @ghidraAddress 0x001b1a40
     */
    int Score(int nFirst, int nSecond);
};
