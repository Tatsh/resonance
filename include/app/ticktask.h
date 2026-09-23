#pragma once

#include <iostream>

#include "app/attachment.h"
#include "mid/mbt.h"
#include "sch/tickclock.h"

/**
 * Task the scheduler runs again every fixed number of MIDI ticks.
 *
 * `8TickTask` in the RTTI descriptor at `0x00901b80`, with Attachment as its one base, and titled
 * after `AppTickTask.cpp`, the translation unit its file-local `Cmd` class records. Its vtable is
 * at `0x007d30c8` and runs five entries. Slot 2 retains Attachment::Destroy() and slot 4 addresses
 * the shared pure-virtual stub at `0x005381a8`, which is what makes the class abstract.
 *
 * The object is 0x20 bytes. The Attachment base supplies the reference count at `+0x00` and the
 * vptr at `+0x04`.
 *
 * The pump at `0x0013aa38` is what fixes the meaning of the members. It subtracts mEpoch from
 * mNextTick, saturates the difference against Mid::MBT's two infinity bounds, hands the result to
 * Tick(), and returns without rescheduling when Tick() reports zero. Otherwise it advances
 * mNextTick by mPeriod, saturates again, and posts a fresh file-local Cmd against the clock.
 *
 * Pitcher, BarSequencer, WahEffector, StutterEffector, GamePowerupPlacer, and the file-local
 * NoteDestroyer of `AppPlaySoundPS2.cpp` all derive from this class.
 *
 * The constructor is the one out-of-line body recovered so far. Print() and the destructor are
 * recorded with their addresses and not written.
 */
class TickTask : public Attachment {
public:
    /**
     * @param pClock The clock the task is posted against.
     * @param nPeriod Ticks between one run and the next.
     * @param nUnknown1c The third argument. Every recovered caller passes zero.
     * @ghidraAddress 0x0013ad88
     */
    TickTask(Sch::TickClock *pClock, int nPeriod, int nUnknown1c);

    /**
     * @ghidraAddress 0x0013adc8
     */
    virtual ~TickTask();

    /**
     * Write the task to a diagnostic stream.
     *
     * Table slot 3. This class writes the literal `{TickTask}` and nothing else. The body is
     * shared with every subclass that does not override it.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0013ac48
     */
    virtual void Print(std::ostream &stream);

    /**
     * Run the task.
     *
     * Table slot 4, and the one pure member of the class.
     *
     * @param nElapsedTicks Ticks between the epoch and the run now due, saturated against
     *                      Mid::MBT's infinity bounds.
     * @return Non-zero to be run again after mPeriod further ticks, zero to stop.
     */
    virtual int Tick(int nElapsedTicks) = 0;

    /**
     * Post the task on its clock.
     *
     * Reads the song position into mNextTick and measures mEpoch from it, then either posts a
     * command that runs the task at the next multiple of mPeriod or, for a task with mUnknown1c
     * clear, hands over to the routine at `0x0013aa38`. Not reconstructed, because that routine
     * and the command class whose table is at `0x007d3080` are unrecovered. The title is
     * inferred.
     *
     * @param nEpochOffset Ticks the epoch lies before the current position, or Mid::MBT's
     *                     infinity sentinel for an epoch at zero.
     * @ghidraAddress 0x0013a860
     */
    void Start(int nEpochOffset);

private:
    Sch::TickClock *mClock; // +0x08 the clock the task is posted against
    // Set to -2 by the constructor and never written again by any recovered routine.
    int mUnknown0c; // +0x0c
    int mPeriod;    // +0x10 ticks between one run and the next
    // Tick the next run is due at, both read and advanced by the pump. Defaults to Mid::MBT's
    // positive infinity sentinel.
    int mNextTick; // +0x14
    // Tick the elapsed count is measured from. Defaults to the same sentinel.
    int mEpoch; // +0x18
    // The constructor's third argument. No recovered routine reads it.
    int mUnknown1c; // +0x1c
};
