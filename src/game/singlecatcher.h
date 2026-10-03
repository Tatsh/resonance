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
 * Its RTTI descriptor is at `0x008f0be0`. It has Catcher as its only base at offset 0. Its primary
 * table is at `0x007e0bb0` with eleven entries and its MsgSource subobject table at `0x007e0b88`
 * with four. The table is the same length as the base's. The class introduces no virtual and
 * overrides only the destructor and the base's two pure slots. The object is 0x84 bytes against the
 * base's 0x80. It adds one word, mLastStep.
 */
class SingleCatcher : public Catcher {
public:
    /**
     * Construct the base with two-bar seeker ranges and record the last step of the play map.
     *
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description.
     * @param pClock The clock the scheduled commands run on.
     * @param tick The scheduler time the base retains.
     * @ghidraAddress NTSC-U/C: 0x001b19b0
     * @ghidraAddress PAL: 0x001b7770
     */
    SingleCatcher(PhraseMgr *pPhraseMgr,
                  Quantizer *pQuantizer,
                  const TrackData *pTrackData,
                  Sch::TickClock *pClock,
                  Sch::Tick tick);

    /**
     * @ghidraAddress NTSC-U/C: 0x001b0d30
     * @ghidraAddress PAL: 0x001b6ae0
     */
    virtual ~SingleCatcher();

    /**
     * Capture the phrase a caught run of bars completes.
     *
     * Slot 9. The routine gives every bar of the step around nBar to mPlayer, totals the points of
     * the nRun bars ending at nBar, and sends a PhraseCapturedMsg (score, the multiplier
     * Player::GetMultiplier() reports for the first bar of the run, and the flag inverted) and a
     * SectionCapturedMsg for the step.
     *
     * @param nBar The last bar of the run.
     * @param nRun The bars the run spans.
     * @param nAutoCatch Non-zero for an automatic catch. The PhraseCapturedMsg receives it inverted
     *                   and the SectionCapturedMsg unchanged.
     * @ghidraAddress NTSC-U/C: 0x001ad630
     * @ghidraAddress PAL: 0x001b3398
     */
    virtual void CapturePhrase(int nBar, int nRun, int nAutoCatch);

    /**
     * Report the caught power bar to this catcher's player.
     *
     * Slot 10. The routine queries PhraseMgr::GetPowerbar() for the bar and, for an answer other
     * than -1, sends a CaughtPowerbarMsg carrying that answer and the player, then delivers the
     * same message to the player directly through MsgSink::Dispatch().
     *
     * @param nBar The caught bar.
     * @ghidraAddress NTSC-U/C: 0x001ad840
     * @ghidraAddress PAL: 0x001b35a8
     */
    virtual void ReportCaughtPowerbar(int nBar);

    /**
     * Give every phrase of the track to one player.
     *
     * CatchingSTG::GivePhrases() reaches it by casting its catcher to this class with
     * `dynamic_cast`. The routine forwards the player to PhraseMgr::ResetOwners() and returns
     * nothing. The title follows that callee and is inferred.
     *
     * @param nTick The song position the caller received, unread.
     * @param pPlayer The player the phrases go to.
     * @ghidraAddress NTSC-U/C: 0x001b1a40
     * @ghidraAddress PAL: 0x001b7800
     */
    void ResetOwners(int nTick, Player *pPlayer);

private:
    int mLastStep; // +0x80, the play map's last step, read once by the constructor
};
