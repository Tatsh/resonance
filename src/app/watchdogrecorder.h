#pragma once

class OBStream;

namespace Sch {
class TimedCommand;
} // namespace Sch

/**
 * Recording of every command the scheduler queues, which Watchdog::BeginRecording() installs.
 *
 * The class is not polymorphic and emits no RTTI, so the title is inferred as the counterpart of
 * WatchdogPlayback. The object is four bytes, the one stream pointer, and is allocated with the
 * untagged allocator.
 */
class WatchdogRecorder {
public:
    /**
     * @param pStream The stream the recording is written to.
     * @ghidraAddress 0x00596290
     */
    explicit WatchdogRecorder(OBStream *pStream);

    /**
     * Write one queued wrapper to the recording.
     *
     * The wrapper is saved and the stream's slot 2 is dispatched after it. Watchdog's delta
     * queueing path is the caller, for a recordable command while recording. The title is
     * inferred.
     *
     * @param pCommand The wrapper.
     * @ghidraAddress 0x005962a0
     */
    void Record(Sch::TimedCommand *pCommand);

private:
    OBStream *mStream; // +0x00
};
