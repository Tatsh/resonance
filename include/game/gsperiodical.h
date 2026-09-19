#pragma once

#include "game/axephrasemaker.h"
#include "sch/cmdid.h"
#include "sch/tickclock.h"

/**
 * Repeating post that re-queues itself one period ahead each time it runs.
 *
 * The class emits no RTTI, because it is not polymorphic. Its title comes from the
 * anonymous-namespace marker `Q235_GLOBAL_$N$GsPeriodical.cppdKuhgb13PeriodicalCmd`, which records
 * the translation unit as `GsPeriodical.cpp` and a file-local command class as `PeriodicalCmd`.
 * The class title is therefore inferred from the file name rather than attested by a descriptor.
 *
 * The object is 0x14 bytes and AxingSTG builds one with `MemAllocScalar(0x14)`. The constructor
 * writes a placeholder at `+0x00`, then overwrites it with the value the phrase maker's table slot
 * 5 reports, which is the constant 6 for AxePhraseMaker.
 *
 * The class is not reconstructed. Only the surface AxingSTG uses is declared, so that it compiles
 * against the real type.
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
    GsPeriodical(Sch::TickClock *pClock, AxePhraseMaker *pPhraseMaker, int nPeriod);

    /**
     * Queue the next run, one period after the current reading.
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

private:
    int mUnknown00;               // +0x00, the value AxePhraseMaker's slot 5 reports
    int mPeriod;                  // +0x04
    Sch::TickClock *mClock;       // +0x08
    CmdID mCommand;               // +0x0c, starts -2
    AxePhraseMaker *mPhraseMaker; // +0x10
};
