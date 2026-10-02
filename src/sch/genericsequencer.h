#pragma once

#include "mid/mbt.h"
#include "sch/cmdid.h"
#include "sch/tickclock.h"

class MsgSink;
class SequencerCmd;

/**
 * Base of the template that dispatches a range of timed messages against a clock.
 *
 * `16GenericSequencer` in the RTTI descriptor at `0x0086f708`, with no base list. It is the base
 * the one Sequencer instantiation in the image derives from at offset 0.
 *
 * Its shape comes from the instantiation's table at `0x007cc7e8`, which runs three entries: the
 * type function, Dispatch(), and the destructor. The destructor is therefore declared second,
 * which MultiMusePlayer's destructor confirms by releasing its sequencer through slot 2 rather
 * than slot 1.
 *
 * The base is 0x1c bytes. Sequencer adds its cursor, the tick of the next message, and its range
 * above that, which the 0x2c-byte allocations in MultiMusePlayer::Start() and BarSequencer::Tick()
 * measure. The member order is the recovered offset order.
 */
class GenericSequencer {
public:
    /**
     * Prepare a sequencer before any post.
     *
     * Inline, and expanded into both allocations. The handle starts unallocated, the command
     * absent, the start tick at kMBTInfinity, and the offset at zero.
     */
    GenericSequencer() : mCommand(nullptr), mOffset(0) {
        mCmdId.mValue = kUnallocatedCommand;
    }

    /**
     * Send the message at the cursor and schedule the next one.
     *
     * Slot 1, pure. SequencerCmd::Execute() runs it. The title is inferred.
     */
    virtual void Dispatch() = 0;

    /**
     * Release the sequencer.
     *
     * Slot 2. The one recovered body is the instantiation's at `0x00100df8`.
     */
    virtual ~GenericSequencer() {
    }

    /**
     * Withdraw every command queued under the handle.
     *
     * Inline. The out-of-line copy at `0x001aa418` sits in the MultiMusePlayer unit, and the
     * instantiation's destructor expands the body. The title is inferred.
     *
     * @ghidraAddress 0x001aa418
     */
    void Withdraw() {
        const CmdID id = mCmdId;
        mClock->Withdraw(id);
    }

protected:
    // The handle value of a command the clock has not queued yet.
    enum { kUnallocatedCommand = -2 };

    // The handle SequencerCmd is queued under.
    CmdID mCmdId;
    // The command that runs Dispatch(), created by the first post.
    SequencerCmd *mCommand;
    // The clock Post() queues against.
    Sch::TickClock *mClock;
    // The clock's song position when Post() ran.
    Mid::MBT mStartTick;
    // Subtracted from every message's position. The constructor builds it from zero through the
    // checking MBT constructor.
    Mid::MBT mOffset;
    // The sink every dispatched message goes to.
    MsgSink *mSink;
};
