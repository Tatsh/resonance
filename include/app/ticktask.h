#pragma once

#include <iostream>

#include "app/attachment.h"
#include "mid/mbt.h"
#include "sch/cmdid.h"
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
 * Run() is what fixes the meaning of the members. It subtracts mEpoch from mNextTick, saturates the
 * difference against Mid::MBT's two infinity bounds, hands the result to Tick(), and returns
 * without rescheduling unless Tick() reports 1. Otherwise it advances mNextTick by mPeriod,
 * saturates again, and posts a fresh file-local Cmd against the clock under mCommand.
 *
 * Pitcher, BarSequencer, WahEffector, StutterEffector, GamePowerupPlacer, and the file-local
 * NoteDestroyer of `AppPlaySoundPS2.cpp` all derive from this class.
 */
class TickTask : public Attachment {
public:
    /**
     * @param pClock The clock the task is posted against.
     * @param nPeriod Ticks between one run and the next.
     * @param bAligned Non-zero to delay the first run to the next multiple of nPeriod. Every
     *                 recovered caller passes zero.
     * @ghidraAddress 0x0013ad88
     */
    TickTask(Sch::TickClock *pClock, int nPeriod, int bAligned);

    /**
     * Withdraw the queued command.
     *
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
     * @return 1 to be run again after mPeriod further ticks, anything else to stop.
     */
    virtual int Tick(int nElapsedTicks) = 0;

    /**
     * Post the task on its clock.
     *
     * Reads the song position into mNextTick and measures mEpoch from it. A task with mAligned
     * clear then runs at once through Run(). An aligned task instead posts a command that runs it
     * at the next multiple of mPeriod. The title is inferred.
     *
     * @param nEpochOffset Ticks the epoch lies before the current position, or Mid::MBT's
     *                     infinity sentinel for an epoch at zero.
     * @ghidraAddress 0x0013a860
     */
    void Start(int nEpochOffset);

    /**
     * Run Tick() for the run now due and post the next one.
     *
     * The file-local Cmd's Execute() is the caller. The title is inferred.
     *
     * @ghidraAddress 0x0013aa38
     */
    void Run();

    /**
     * Withdraw the queued command and forget its handle.
     *
     * The destructor, GamePowerupPlacer, StutterEffector, PitchingSTG, and VoxingSTG call it. The
     * title is inferred.
     *
     * @ghidraAddress 0x0013ae10
     */
    void Stop();

private:
    Sch::TickClock *mClock; // +0x08 the clock the task is posted against
    CmdID mCommand;         // +0x0c the handle of the queued command, -2 while none is queued
    int mPeriod;            // +0x10 ticks between one run and the next
    // Tick the next run is due at, both read and advanced by Run(). Defaults to Mid::MBT's
    // positive infinity sentinel.
    int mNextTick; // +0x14
    // Tick the elapsed count is measured from. Defaults to the same sentinel.
    int mEpoch; // +0x18
    // Non-zero when the first run waits for the next multiple of mPeriod.
    int mAligned; // +0x1c
};
