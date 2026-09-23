#pragma once

#include "game/catcher.h"
#include "game/quantizer.h"
#include "game/trackdata.h"
#include "gs/phrasemgr.h"
#include "sch/tick.h"
#include "sch/tickclock.h"

/**
 * Catcher that scores a whole range of bars at once.
 *
 * `12MultiCatcher` in the RTTI descriptor at `0x008ef518`, with Catcher as its only base at offset
 * 0. Its primary table is at `0x007e0b28` with eleven entries and its MsgSource subobject table at
 * `0x007e0b00` with four. The table is the same length as the base's, so the class introduces no
 * virtual and overrides only the destructor and the base's two pure slots. The object is 0x80
 * bytes, the same as the base, so it adds no data member.
 *
 * The constructor's whole body is the base construction and the two table stores, and it supplies
 * 1 for the base's `int` parameter. The destructor's whole body is the base teardown and the
 * tagged release behind the deleting flag, which is why both definitions are empty of anything a
 * programmer wrote.
 *
 * CatchingSTG builds one of these for every game mode other than 1, and a SingleCatcher for 1.
 */
class MultiCatcher : public Catcher {
public:
    /**
     * @param pPhraseMgr The phrase manager for the track.
     * @param pQuantizer The quantiser for the track.
     * @param pTrackData The track description.
     * @param pClock The clock the scheduled commands run on.
     * @param tick The scheduler time the base retains.
     * @ghidraAddress 0x001b1a60
     */
    MultiCatcher(PhraseMgr *pPhraseMgr,
                 Quantizer *pQuantizer,
                 const TrackData *pTrackData,
                 Sch::TickClock *pClock,
                 Sch::Tick tick);

    /**
     * @ghidraAddress 0x001b0d98
     */
    virtual ~MultiCatcher();

    /**
     * Capture the phrase of one bar.
     *
     * Slot 9. The bar's points, times Player::Slot16() for the bar (or 1 for an automatic
     * catch), become its score. The bar is given to mPlayer, a PhraseCapturedMsg whose run starts
     * nSecond - 1 bars earlier and a SectionCapturedMsg are sent, and the score is stored as the
     * bar's phrase byte. A bar with a power bar also sends a CaughtPowerbarMsg, to the sinks and
     * to mPlayer, unless the catch was automatic.
     *
     * @param nFirst The bar.
     * @param nSecond The bars the run spans.
     * @param nThird Non-zero for an automatic catch.
     * @ghidraAddress 0x001ad8e8
     */
    virtual void Slot9(int nFirst, int nSecond, int nThird);

    /**
     * Slot 10. The body is empty. The base declares the slot pure, so the empty body is a real
     * override rather than an inherited default, and it is what makes this class concrete.
     *
     * @param nBar The caught bar, unread.
     * @ghidraAddress 0x001b0e00
     */
    virtual void Slot10(int nBar);
};
