#pragma once

#include <iostream>

#include "app/attachment.h"
#include "sch/cmdid.h"

class WatchdogTimer;

/**
 * Task the scheduler runs again every fixed number of nanoseconds.
 *
 * `8TimeTask` in the RTTI descriptor, with Attachment as its one base, and titled after
 * `AppTimeTask.cpp`, the translation unit its file-local `Cmd` class records. It is the
 * nanosecond counterpart of TickTask. Its vtable is at `0x007d31a8`, and slot 4, Tick(), is pure.
 * `Synth::Setup::SynthFade` is the one recovered subclass.
 *
 * The object is 0x28 bytes. The Attachment base supplies the reference count at `+0x00` and the
 * vptr at `+0x04`.
 */
class TimeTask : public Attachment {
public:
    /**
     * @param pClock The clock the task is posted against.
     * @param nPeriodNs Nanoseconds between one run and the next.
     * @ghidraAddress 0x0013b100
     */
    TimeTask(WatchdogTimer *pClock, long long nPeriodNs);

    /**
     * Withdraw the queued command.
     *
     * @ghidraAddress 0x0013b138
     */
    virtual ~TimeTask();

    /**
     * Write the literal `{TimeTask}` to a diagnostic stream.
     *
     * Table slot 3.
     *
     * @param stream The stream to write to.
     * @ghidraAddress 0x0013afc0
     */
    virtual void Print(std::ostream &stream);

    /**
     * Run the task.
     *
     * Table slot 4, and the one pure member of the class.
     *
     * @param nElapsedNs Nanoseconds between the epoch and the run now due.
     * @return 1 to be run again after mPeriodNs further nanoseconds, anything else to stop.
     */
    virtual int Tick(long long nElapsedNs) = 0;

    /**
     * Start the task now and run it at once.
     *
     * Reads the clock into mNextNs and measures mEpochNs from it, then runs Run(). The title is
     * inferred.
     *
     * @param nEpochOffsetNs Nanoseconds the epoch lies before now, or kNoEpochOffset for an epoch
     *                       at zero.
     * @ghidraAddress 0x0013b180
     */
    void Start(long long nEpochOffsetNs);

    /**
     * Run Tick() for the run now due and post the next one.
     *
     * The file-local Cmd's Execute() is the caller. The title is inferred.
     *
     * @ghidraAddress 0x0013ae70
     */
    void Run();

    /**
     * Withdraw the queued command and forget its handle.
     *
     * The title is inferred.
     *
     * @ghidraAddress 0x0013b1f0
     */
    void Stop();

    /** The epoch offset Start() treats as an epoch at zero. */
    static constexpr long long kNoEpochOffset = -1000000;

private:
    WatchdogTimer *mClock; // +0x08 the clock the task is posted against
    CmdID mCommand;        // +0x0c the handle of the queued command, -2 while none is queued
    long long mPeriodNs;   // +0x10
    long long mNextNs;     // +0x18 the time the next run is due at
    long long mEpochNs;    // +0x20 the time the elapsed count is measured from
};
