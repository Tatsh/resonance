#pragma once

#include "sch/tick.h"

class CmdID;
class Watchdog;

namespace Sch {
class Command;
} // namespace Sch

/**
 * Time base a scheduler measures its due times against, and the base class of Sch::TickClock.
 *
 * The class is not polymorphic and has no RTTI, and no literal titles it. The title here is
 * retained from an earlier pass rather than attested, and it understates the class: this 0x18-byte
 * object is the base of Sch::TickClock, and the bodies at `0x004a7828`, `0x004a7848`, `0x004a7878`,
 * and `0x004a77c0` are single bodies shared between the two rather than routines of a monitor.
 *
 * The image does not include the strings `Watchdog`, `Scheduler`, or `Timer`, and its only `Sch`
 * names are the command classes, the tempo map, the tick, and the clock itself. The sequel
 * Amplitude (SCUS_972.58) is the best evidence for a replacement title. Its RTTI records a class
 * `Timer` (`5Timer` and `P5Timer` in template and pointer type names) beside a class `Scheduler`
 * with nested `CommandInfo`, `ByCommand`, `ByID`, and `CancelPred` types. Amplitude has no `Sch`
 * namespace, no TimedCommand, and no TempoMap. Together with this image's `Sch` namespace, that
 * record makes `Sch::Timer` the likely original title of this class. The title is inferred rather
 * than attested, and it is not applied yet.
 *
 * The origin is stored negated and may be set only once, so a second SetOrigin() is ignored. Every
 * member is public, because Sch::TickClock reads each one directly and the image has no accessor.
 */
class WatchdogTimer {
public:
    /**
     * @param pWatchdog The monitor these readings belong to.
     * @ghidraAddress 0x004a7780
     */
    WatchdogTimer(Watchdog *pWatchdog);

    /**
     * Fix the origin of the time base.
     *
     * The first call wins. Every later call is ignored.
     *
     * @param nNanoseconds The reading to treat as zero.
     * @ghidraAddress 0x004a7828
     */
    void SetOrigin(long long nNanoseconds);

    /**
     * Report the current time.
     *
     * Before an origin is set the reading is mPausedNs. Afterwards it is the monitor's
     * most recent due tick relative to the origin.
     *
     * @return The time in nanoseconds.
     * @ghidraAddress 0x004a77c0
     */
    long long Now();

    /**
     * Stop the time base at its current reading.
     *
     * Clears mHasOrigin and stores the reading Now() would have reported in mPausedNs. Now()
     * reports mPausedNs from then on. Does nothing while mHasOrigin is already clear.
     * GrooveWorld::StopLevel() at `0x0018ec90` calls it on the song clock. The title is inferred.
     *
     * @ghidraAddress 0x004a7878
     */
    void Pause();

    /**
     * Restart the time base from the reading Pause() stopped it at.
     *
     * Sets mHasOrigin and stores the origin that makes Now() continue from mPausedNs. Does
     * nothing while mHasOrigin is already set. GrooveWorld::StartPlay() calls it on the song clock.
     * The title is inferred.
     *
     * @ghidraAddress 0x004a7848
     */
    void Resume();

    /**
     * Queue a command, choosing between the absolute and the delta path.
     *
     * A non-zero bDelta takes the delta path, which resolves the due tick against the later of the
     * scheduler's current time and its clock. A zero bDelta takes the absolute path, which
     * subtracts this clock's origin instead and passes a zero recordable flag. Two facts place it
     * on this class rather than on Sch::TickClock. The body reads only mNegatedOrigin and
     * mWatchdog, and TimeTask posts through it on the plain timer that Globals::InitServices()
     * builds.
     *
     * @param pCommand The command to run.
     * @param tick The tick the caller requests.
     * @param id The handle to queue under, allocated when it is still -2.
     * @param bRecordable Non-zero for a post the recorded stream is to include. Recording writes
     *                    such a post out and playback suppresses it, because the stream supplies
     *                    it instead.
     * @param bDelta Non-zero to treat tick as a distance from now.
     * @ghidraAddress 0x004a78b8
     */
    void Post(Sch::Command *pCommand, Sch::Tick tick, CmdID &id, int bRecordable, int bDelta);

    /**
     * Withdraw the first wrapper queued under a handle.
     *
     * Forwards a copy of the handle to Watchdog::WithdrawByCmdID(). The body reads only
     * mWatchdog.
     *
     * @param id The handle to withdraw.
     * @ghidraAddress 0x004a79d0
     */
    void Withdraw(const CmdID &id);

    /**
     * Queue a command a distance from now, under a handle the caller retains.
     *
     * The body reads only mWatchdog, which is what places it on this class rather than on
     * Sch::TickClock. GameRecorder::ScheduleEnd() calls it on the plain WatchdogTimer that
     * Globals creates.
     *
     * @param pCommand The command to run.
     * @param tick The distance from now.
     * @param id The handle to queue under.
     * @param bRecordable Non-zero for a post the recorded stream is to include.
     * @ghidraAddress 0x004a60a0
     */
    void PostIn(Sch::Command *pCommand, Sch::Tick tick, CmdID &id, int bRecordable);

    /**
     * Queue a command a distance from now, discarding the handle.
     *
     * The body reads only mWatchdog, as the keyed overload does.
     *
     * @param pCommand The command to run.
     * @param tick The distance from now.
     * @ghidraAddress 0x004a6178
     */
    void PostIn(Sch::Command *pCommand, Sch::Tick tick);

    /**
     * The origin, negated, in nanoseconds.
     *
     * Sch::TickClock::PostAt() and both PostAtSongTick() overloads subtract it to express a due
     * time in the scheduler's frame.
     */
    long long mNegatedOrigin;

    /**
     * The reading Now() reports while no origin is set, in nanoseconds.
     *
     * Sch::TickClock::SetSongTick() writes it.
     */
    long long mPausedNs;

    /** Non-zero while the time base runs against the scheduler. */
    int mHasOrigin;

    /**
     * The scheduler whose current time the readings follow.
     *
     * Sch::TickClock::PostAt() and both PostAtSongTick() overloads queue through it.
     */
    Watchdog *mWatchdog;
};
