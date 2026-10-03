#pragma once

#include <vector>

class IBStream;

namespace Sch {

class Scheduler;
class TimedCommand;

/**
 * Replay of a recorded command stream that Sch::Scheduler::StartPlayback() installs.
 *
 * The class is not polymorphic and emits no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The object is 0x14 bytes and is allocated with the untagged
 * allocator. Load() reads the recorded wrappers into mCommands, stopping at the EndRecordingCmd
 * that ends a recording, and Play() rewinds the scheduler's clock and queues every wrapper on the
 * scheduler.
 */
class Playbacker {
public:
    /**
     * @param pWatchdog The scheduler the commands are replayed on.
     * @ghidraAddress NTSC-U/C: 0x00594968
     * @ghidraAddress PAL: 0x005d7d00
     */
    explicit Playbacker(Scheduler *pWatchdog);

    /**
     * Release every loaded wrapper through Attachment::ReleaseIfSet() and empty mCommands.
     *
     * @ghidraAddress NTSC-U/C: 0x00594988
     * @ghidraAddress PAL: 0x005d7d20
     */
    ~Playbacker();

    /**
     * Read the recorded wrappers from a stream.
     *
     * Empties mCommands, then reads one Sch::TimedCommand after another until the stream reaches
     * its end or a wrapper holds a command whose CmdID() is 6, the identifier of EndRecordingCmd.
     * The wrapper that stops the loop is deleted. Each wrapper kept has its handle reserved
     * through Sch::CmdID::ReserveID(). mCursor is left at the first wrapper. The title is inferred.
     *
     * @param stream The recording.
     * @ghidraAddress NTSC-U/C: 0x00594a78
     * @ghidraAddress PAL: 0x005d7e10
     */
    void Load(IBStream &stream);

    /**
     * Rewind the scheduler's clock and queue every loaded wrapper.
     *
     * Pauses the clock, restarts it at zero, queues each wrapper from mCursor on, and resumes the
     * clock.
     *
     * @ghidraAddress NTSC-U/C: 0x005962e8
     * @ghidraAddress PAL: 0x005d96f0
     */
    void Play();

private:
    // Hands every wrapper from mCursor to the end to Scheduler::QueueReplayed().
    // NTSC-U/C: 0x00596330, PAL: 0x005d9738
    void QueueRemaining();

    std::vector<Sch::TimedCommand *> mCommands;         // +0x00
    std::vector<Sch::TimedCommand *>::iterator mCursor; // +0x0c
    Scheduler *mWatchdog;                               // +0x10
};

} // namespace Sch
