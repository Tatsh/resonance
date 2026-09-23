#pragma once

#include "sch/cmdid.h"

class PhraseMaker;

namespace Sch {
class TickClock;
} // namespace Sch

/**
 * Repeating post that re-queues itself one period ahead each time it runs.
 *
 * The class emits no RTTI, because it is not polymorphic. Its title comes from the
 * anonymous-namespace marker `Q235_GLOBAL_$N$GsPeriodical.cppdKuhgb13PeriodicalCmd`, which records
 * the translation unit as `GsPeriodical.cpp` and a file-local command class as `PeriodicalCmd`.
 * The class title is therefore inferred from the file name rather than attested by a descriptor.
 *
 * The object is 0x14 bytes and AxingSTG builds one with the global `operator new(0x14)`. The
 * constructor writes kMBTInfinity to mOrigin, then overwrites it with the origin the phrase maker's
 * slot 5 reports, which is the constant 6 for AxePhraseMaker.
 *
 * Every song position the class computes is clamped to the finite range and passed through the
 * discarded finiteness test, which is the expansion of an inline position type rather than
 * arithmetic the class performs itself.
 *
 * Every member is private. Only this class and its file-local command address them.
 */
class GsPeriodical {
public:
    /**
     * @param pClock The clock the post runs on.
     * @param pPhraseMaker The phrase maker the post drives.
     * @param nPeriod The period in MIDI ticks. AxingSTG passes 0x780, one bar at 480 per quarter
     *                note.
     * @ghidraAddress 0x001b4738
     */
    GsPeriodical(Sch::TickClock *pClock, PhraseMaker *pPhraseMaker, int nPeriod);

    /**
     * Queue the first run, one period after the origin.
     *
     * @ghidraAddress 0x001b4870
     */
    void Post();

    /**
     * Withdraw the queued run.
     *
     * @ghidraAddress 0x001b48f8
     */
    void Withdraw();

    /**
     * Report the period that has elapsed at a song position and queue the next run.
     *
     * The phrase maker's slot 4 receives the distance from the origin divided by the period.
     * PeriodicalCmd::Execute() is the one caller.
     *
     * @param nTick The song position the run was queued for, in MIDI ticks.
     * @ghidraAddress 0x001b45d0
     */
    void Run(int nTick);

private:
    // 0x001b4548
    void PostAt(int nTick);

    int mOrigin;               // +0x00, the phrase maker's slot 5
    int mPeriod;               // +0x04
    Sch::TickClock *mClock;    // +0x08
    CmdID mCommand;            // +0x0c, starts -2
    PhraseMaker *mPhraseMaker; // +0x10
};
