#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "msg/message.h"

/**
 * Base of the per-instrument phrase makers.
 *
 * `11PhraseMaker` in the RTTI descriptor at `0x008f0300`, over MsgSink at offset 0 and MsgSource at
 * offset 4. AxePhraseMaker derives from it at offset 0.
 *
 * The primary table at `0x007ddc88` runs six entries. Slot 3, MsgSink::HandleMessage(), and slot
 * 4 address the shared pure-virtual stub at `0x005381a8`, and slot 5 has a body of its own, so the
 * class introduces two virtuals and is abstract. The MsgSource table at `0x007dddf0` adjusts
 * `this` by `-4` and retains AddSink() and RemoveSink(). GsPeriodical drives both new slots: it
 * reads slot 5 once as its origin and calls slot 4 once per period with the period index.
 *
 * The destructor at `0x0019d2a0` is implicitly declared. It restores the base tables, frees
 * MsgSource's vector, and releases the object under MsgSink's tag, which is what the compiler
 * generates for a class with no member of its own. The implicit default constructor is emitted
 * out of line at `0x0019d1b0`, where AxePhraseMaker's constructor calls it.
 */
class PhraseMaker : public MsgSink, public MsgSource {
public:
    /**
     * Act on a period that has elapsed. Slot 4, pure. The verb is unrecovered.
     *
     * AxePhraseMaker's override at `0x0019d990` is the one recovered body.
     *
     * @param nPeriod The index of the period, counted from the origin slot 5 reports.
     */
    virtual void Slot4(int nPeriod) = 0;

    /**
     * Report the song position periods are counted from. Slot 5. The verb is unrecovered.
     *
     * The body here discards the finiteness test on zero and returns zero.
     *
     * @return The origin, in MIDI ticks.
     * @ghidraAddress 0x0019d370
     */
    virtual int Slot5();
};
