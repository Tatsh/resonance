#pragma once

#include <vector>

class IBStream;
class Watchdog;

namespace Sch {
class TimedCommand;
} // namespace Sch

/**
 * Replay of a recorded command stream that Watchdog::StartPlayback() installs.
 *
 * The class is not polymorphic and emits no RTTI, so the title is inferred from the one routine
 * that creates it. The object is 0x14 bytes and is allocated with the untagged allocator. Load()
 * reads the recorded wrappers into mCommands, stopping at the EndRecordingCmd that ends a
 * recording, and Start() rewinds the watchdog's clock and queues every wrapper on the watchdog.
 */
class WatchdogPlayback {
public:
    /**
     * @param pWatchdog The monitor the commands are replayed on.
     * @ghidraAddress 0x00594968
     */
    explicit WatchdogPlayback(Watchdog *pWatchdog);

    /**
     * Read the recorded wrappers from a stream.
     *
     * Empties mCommands, then reads one Sch::TimedCommand after another until the stream fails or
     * a wrapper holds a command whose CmdID() is 6, the identifier of EndRecordingCmd. Each
     * wrapper kept has its handle reserved through `0x005e5908`. mCursor is left at the first
     * wrapper. Not reconstructed, because Sch::TimedCommand has no default constructor declared
     * and the handle reservation is unrecovered. The title is inferred.
     *
     * @param stream The recording.
     * @ghidraAddress 0x00594a78
     */
    void Load(IBStream &stream);

    /**
     * Rewind the watchdog's clock and queue every loaded wrapper.
     *
     * Pauses the clock, restarts it at zero, queues each wrapper from mCursor on, and resumes the
     * clock. The title is inferred.
     *
     * @ghidraAddress 0x005962e8
     */
    void Start();

private:
    // 0x00596330. Hands every wrapper from mCursor to the end to Watchdog::QueueReplayed().
    void QueueRemaining();

    std::vector<Sch::TimedCommand *> mCommands;         // +0x00
    std::vector<Sch::TimedCommand *>::iterator mCursor; // +0x0c
    Watchdog *mWatchdog;                                // +0x10
};
