#pragma once

#include "sch/tickclock.h"

/**
 * Base of the template that dispatches a range of timed objects.
 *
 * `16GenericSequencer` in the RTTI descriptor at `0x0086f708`, with no base list. It is the base
 * the one Sequencer instantiation in the image derives from at offset 0.
 *
 * Its shape comes from the instantiation's table at `0x007cc7e8`, which runs three entries: the
 * type function, one virtual, and the destructor. The destructor is therefore declared second,
 * which MultiMusePlayer's destructor confirms by releasing its sequencer through slot 2 rather
 * than slot 1.
 *
 * The base is 0x24 bytes. Sequencer adds its two range pointers above that, which the 0x2c-byte
 * allocation in MultiMusePlayer::Start() measures. No accessor of the class is recovered, so every
 * field stays a placeholder.
 *
 * The verb of slot 1 is unrecovered. No routine in the image calls it directly.
 */
class GenericSequencer {
public:
    /**
     * Slot 1, verb unrecovered.
     *
     * @ghidraAddress 0x00100e70
     */
    virtual void Slot1() = 0;

    /**
     * @ghidraAddress 0x00100df8
     */
    virtual ~GenericSequencer();

    /**
     * Handle the sequencer is queued under, set to -2 by every recovered construction.
     *
     * Public because WithdrawSchedulerCommand() reads it from outside the class and the image
     * exposes no accessor. A friend declaration fits equally well. +0x04
     */
    int mCmdId;

private:
    int mUnknown08; // +0x08 cleared on construction

public:
    /**
     * The clock the sequencer is posted against, written by PostSequencer().
     *
     * Public for the same reason as mCmdId. The type comes from
     * WithdrawSchedulerCommand() calling Sch::TickClock::Withdraw() on it. +0x0c
     */
    Sch::TickClock *mClock;

private:
    // Defaults to Mid::MBT's positive infinity sentinel.
    int mUnknown10; // +0x10
    int mUnknown14; // +0x14 cleared on construction
    int mUnknown18; // +0x18 never written by a recovered routine

public:
    /**
     * Cursor into the range, written by PostSequencer() and advanced as the range is dispatched.
     *
     * Public because MultiMusePlayer::PlayerFinished() compares it against the range's finish from
     * outside the hierarchy and the image exposes no accessor. A friend declaration fits equally
     * well. +0x1c
     */
    void *mCursor;

private:
    // Defaults to the same sentinel as mUnknown10.
    int mUnknown20; // +0x20
};
