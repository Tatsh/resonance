#pragma once

#include "app/msgsink.h"
#include "app/msgsource.h"
#include "app/ticktask.h"
#include "sch/tickclock.h"

/**
 * Abstract tick-driven task that both receives and sends messages.
 *
 * `7Pitcher` in the RTTI descriptor at `0x00901c60`, with MsgSink at offset 0, MsgSource at offset
 * 4, and TickTask at offset 24. The class declares no data member of its own, which its destructor
 * proves by tearing down only the two subobjects, so it is 0x38 bytes and every derived class's
 * data starts at `+0x38`.
 *
 * Its three tables are at `0x007e1520`, `0x007e14f8`, and `0x007e14c8`. Two slots address the
 * shared pure-virtual stub: MsgSink::HandleMessage() in the primary table and TickTask::Tick() in
 * the TickTask table. It therefore declares no virtual of its own and leaves two of its bases'
 * pure, which is what makes it abstract.
 *
 * Voxer, Scratcher, and NotePitcher derive from it and each overrides exactly those two slots. All
 * three override Tick() by dividing the elapsed tick count by a stored divisor, so a Pitcher turns
 * a tick rate into a position within a bar.
 *
 * g++ 2.9x emitted this class's three tables into each of the three deriving translation units, at
 * `0x007e14c8`, `0x007e57e0`, and `0x007e61d0`, which is a second measurement of how many
 * subclasses there are.
 *
 * The constructor is inlined into each subclass's constructor and has no out-of-line copy. Each
 * such emission constructs the MsgSource subobject, constructs the TickTask subobject with the
 * clock and a period of 1920 ticks, and then installs this class's three tables before the
 * subclass installs its own.
 */
class Pitcher : public MsgSink, public MsgSource, public TickTask {
public:
    /**
     * Construct the three bases, the TickTask one to run every bar.
     *
     * Inline. Each subclass constructor expands it, starting with the finiteness test of the
     * period.
     *
     * @param pClock The clock the task is posted against.
     */
    explicit Pitcher(Sch::TickClock *pClock) : TickTask(pClock, Mid::MBT(kBarPeriod).mTick, 0) {
    }

    /**
     * Inline. Identical copies sit at `0x001b3348`, `0x001d10b8`, and `0x001d9798`, one in each
     * deriving translation unit, and the deriving destructors all call the first.
     *
     * @ghidraAddress 0x001b3348
     */
    virtual ~Pitcher() {
    }

    /** The TickTask period, one bar of 1920 ticks. */
    static constexpr int kBarPeriod = 1920;
};
