#pragma once

#include <set>

#include "app/watchdogclock.h"
#include "sch/cmdid.h"
#include "sch/timedcommand.h"

class IBStream;
class OBStream;
class WatchdogPlayback;
class WatchdogRecorder;

/**
 * Scheduler that runs queued commands when their due time arrives.
 *
 * The class is not polymorphic and has no RTTI, and no literal in the image identifies it. The
 * title here is retained from an earlier pass rather than attested. The image does not include the
 * strings `Watchdog` or `Scheduler`, and its four `Sch` names (Sch::Command, Sch::TempoMap,
 * Sch::TickClock, and Sch::TimedCommand) do not include a scheduler.
 *
 * The sequel Amplitude (SCUS_972.58) is the best evidence for a replacement title. Its RTTI records
 * a class `Scheduler` with the nested types `Scheduler::CommandInfo`, `Scheduler::ByCommand`,
 * `Scheduler::ByID`, and `Scheduler::CancelPred`, and red-black tree nodes of `CommandInfo`, beside
 * a time-base class `Timer`. Amplitude has no `Sch` namespace, no TimedCommand, and no TempoMap.
 * Together with this image's `Sch` namespace, that record makes `Sch::Scheduler` the likely
 * original title of this class. The title is inferred rather than attested, and it is not applied
 * yet.
 *
 * What the class does is measured rather than inferred. The member at `+0x00` is the one pointer of
 * a red-black tree of Sch::TimedCommand pointers, whose header node the constructor takes from the
 * container pool at `0x00667080` and self-links through `+0x08` and `+0x0c`. The ordering predicate
 * at `0x004ac4f8` reads the due tick as a signed 64-bit quantity and falls back to the order member
 * as unsigned.
 *
 * Service() walks that tree from the left, stops as soon as the front entry is still in the future,
 * runs the wrapper it takes, clears the command's queued flag, and releases the wrapper. It does
 * not observe progress and it reports nothing.
 *
 * The threshold of six seconds in Service() is not a stall limit. When the distance from the last
 * reading exceeds it, the clock is marked back to the current time and re-read, which stops the
 * loop from running six seconds of arrears in one burst.
 *
 * The member at `+0x0c` is a stream mode, one for recording and two for playback, and `+0x10` is
 * the WatchdogRecorder that BeginRecording() installs. The member at `+0x48` blocks every
 * queueing path while it is set.
 *
 * mClock is public because MainLoop::Poll() reads its origin directly and the image has no
 * accessor for it. It sits amid unrecovered words, so the members below are grouped by access
 * rather than by offset, and each trailing comment records the real offset.
 */
class Watchdog {
public:
    /**
     * Order of the command queue: by due tick, then by order as an unsigned quantity.
     *
     * The comparison is recovered from the queue's insert at `0x004acd60` and find at `0x004ac4f8`,
     * both template instantiations.
     */
    struct QueueOrder {
        /**
         * Report whether one wrapper runs before another.
         *
         * @param pLeft The first wrapper.
         * @param pRight The second wrapper.
         * @return True when pLeft sorts first.
         */
        bool operator()(const Sch::TimedCommand *pLeft, const Sch::TimedCommand *pRight) const {
            if (pLeft->mDueTick.mValue == pRight->mDueTick.mValue) {
                return static_cast<unsigned>(pLeft->mOrder) < static_cast<unsigned>(pRight->mOrder);
            }
            return pLeft->mDueTick.mValue < pRight->mDueTick.mValue;
        }
    };

    /**
     * Construct an idle scheduler with an empty queue and a fresh clock.
     *
     * @ghidraAddress 0x004a9858
     */
    Watchdog();

    /**
     * Block queueing, release every queued wrapper, and release the recorder and the playback.
     *
     * @ghidraAddress 0x004a9980
     */
    ~Watchdog();

    /**
     * Start recording every recordable command queued from now on.
     *
     * Installs a WatchdogRecorder over the stream, sets mStreamMode to 1, and resets the clock and
     * the current time to zero. GameRecorder is the caller. The title is retained from an earlier
     * pass.
     *
     * @param stream The stream the recording is written to.
     * @ghidraAddress 0x004ac8c0
     */
    void BeginRecording(OBStream &stream);

    /**
     * Queue a wrapper at an absolute scheduler time.
     *
     * Performs no work while queueing is blocked. Otherwise the wrapper takes the order and the
     * due tick, the handle is allocated when it still reads -2 and copied into the wrapper, and the
     * wrapper is queued with a reference of the queue's own. The recordable flag is not read on
     * this path. Every Sch::TickClock post that resolves an absolute time is a caller.
     *
     * @param pCommand The wrapper.
     * @param nTick The due time in nanoseconds.
     * @param id The handle, allocated in place when it reads -2.
     * @param bRecordable Unread.
     * @param nOrder The order among wrappers due at the same time.
     * @ghidraAddress 0x004ac608
     */
    void QueueAbsolute(
        Sch::TimedCommand *pCommand, long long nTick, CmdID &id, int bRecordable, int nOrder);

    /**
     * Queue a wrapper at a distance from the current time.
     *
     * A recordable command is dropped during playback, because the recording supplies it. The due
     * time is the later of the current time and the clock reading, plus nDelta, and queueing then
     * proceeds as in QueueAbsolute(), except that the command is marked queued and, while
     * recording, a recordable wrapper is written to the recording.
     *
     * @param pCommand The wrapper.
     * @param nDelta The distance from now in nanoseconds.
     * @param id The handle, allocated in place when it reads -2.
     * @param bRecordable Non-zero for a command the recording carries.
     * @param nOrder The order among wrappers due at the same time.
     * @ghidraAddress 0x004ac698
     */
    void QueueDelta(
        Sch::TimedCommand *pCommand, long long nDelta, CmdID &id, int bRecordable, int nOrder);

    /**
     * Build a wrapper for a command while recording, and discard it.
     *
     * While recording, a wrapper is built on the stack with the current time and a fresh handle,
     * and then destroyed without being queued or recorded. The image has no caller. The title is
     * retained from an earlier pass.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x004ac808
     */
    void PostUnreferenced(Sch::Command *pCommand);

    /**
     * Queue a command the playback reader supplies.
     *
     * The command is marked queued, and the wrapper is queued with a reference of the queue's own
     * unless queueing is blocked. The due tick is the one the recording stored. WatchdogPlayback's
     * start routine is the caller. The title is retained from an earlier pass.
     *
     * @param pCommand The wrapper to queue.
     * @ghidraAddress 0x004ac7b0
     */
    void QueueReplayed(Sch::TimedCommand *pCommand);

    /**
     * Withdraw the first queued wrapper whose handle matches.
     *
     * A handle that is not positive withdraws nothing. The walk stops at the first match, which is
     * erased and released.
     *
     * @param id The handle.
     * @ghidraAddress 0x004aa260
     */
    void WithdrawByCmdID(const CmdID &id);

    /**
     * Withdraw the entry at a wrapper's position in the queue.
     *
     * The entry erased is the one that sorts equivalent to pCommand (the same due tick and order),
     * which need not be pCommand itself, and the reference released is pCommand's. Nothing is
     * erased or released when no entry is equivalent. The image has no caller. The
     * title is inferred.
     *
     * @param pCommand The wrapper whose position is withdrawn.
     * @ghidraAddress 0x004a9d00
     */
    void Withdraw(Sch::TimedCommand *pCommand);

    /**
     * Reset the clock to zero and the current time with it.
     *
     * WatchdogPlayback's start routine calls this before queueing a recording. The title is
     * inferred.
     *
     * @ghidraAddress 0x004aca30
     */
    void RestartClock();

    /**
     * Release the recorder and the playback and leave both stream modes.
     *
     * GameRecorder's end of recording and GamePlayback's destructor are the callers.
     *
     * @ghidraAddress 0x004ac9e8
     */
    void Close();

    /**
     * Start replaying a recorded command stream.
     *
     * Installs a WatchdogPlayback in mPlayback, has it read the stream, sets mStreamMode to 2,
     * and starts it. GamePlayback's constructor is the caller. The title is inferred.
     *
     * @param stream The recording, positioned after the session state.
     * @ghidraAddress 0x004ac950
     */
    void StartPlayback(IBStream &stream);

    /**
     * Run every queued command whose due time has arrived.
     *
     * The loop takes the leftmost entry of the queue, stops once that entry is still in the future,
     * erases it, copies its due tick into the scheduler's current time, dispatches
     * Sch::TimedCommand::Run(), clears the command's queued flag, and releases the wrapper. A
     * std::exception that escapes a command is shown through ShowReportedMessage() for 50 units
     * and the loop continues.
     *
     * Before the loop, g_llWatchdogSecondNs catches up with the clock when more than a second
     * behind, and a clock more than six seconds past the current time is marked back to it and
     * re-read. When the loop stops, the current time advances to the clock reading if that is
     * later.
     *
     * @ghidraAddress 0x004aa848
     */
    void Service();

    /**
     * Mark the clock at the current time and record the reading in g_llWatchdogSecondNs.
     *
     * @ghidraAddress 0x004aca60
     */
    void Flush();

    /**
     * Empty the command queue, releasing every wrapper it held.
     *
     * The queue is copied, emptied, and the copy's wrappers are released one by one, so a release
     * that reaches back into the scheduler finds the queue already empty. The title is retained
     * from an earlier pass.
     *
     * @ghidraAddress 0x004a9a78
     */
    void Snapshot();

    /** Clock every due time is measured against. `+0x20` */
    WatchdogClock mClock;

private:
    // WatchdogTimer::Now() reads mNowNs.
    friend class WatchdogTimer;

    // The tail both queueing paths expand: allocate the handle while it still reads -2, copy it
    // into the wrapper, and queue the wrapper with a reference of the queue's own.
    void Enqueue(Sch::TimedCommand *pCommand, CmdID &id);

    // Every queued wrapper, each holding one reference the queue gives back when it runs or is
    // withdrawn.
    std::multiset<Sch::TimedCommand *, QueueOrder> mQueue; // +0x00
    // +0x0c 1 while recording, 2 while playing back; released by Close().
    int mStreamMode;
    // The recording BeginRecording() installs, released by Close().
    WatchdogRecorder *mRecorder; // +0x10
    // The replay StartPlayback() installs, released by Close().
    WatchdogPlayback *mPlayback; // +0x14
    long long mNowNs;            // +0x18 the due tick of the command most recently run
    int mBlocked;                // +0x48 blocks every queueing path while set
    int mUnknown4c;              // +0x4c
};

/**
 * Clock reading Watchdog::Service() refreshes at most once a second, and Watchdog::Flush() sets.
 *
 * Only those two routines touch it. Nothing reads it outside Service().
 *
 * @ghidraAddress 0x006f8a80
 */
extern long long g_llWatchdogSecondNs;
