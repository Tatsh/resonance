#pragma once

class OBStream;

namespace Sch {

class TimedCommand;

/**
 * Recording of every command the scheduler queues, installed by Sch::Scheduler::BeginRecording().
 *
 * The class is not polymorphic and emits no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The demo's constructor has the same instructions as this class's
 * constructor, and the demo's Record() differs from this class's Record() only by a logging block
 * before the same save and stream call. The object is four bytes, the one stream pointer, and is
 * allocated with the untagged allocator.
 */
class Recorder {
public:
    /**
     * @param pStream The stream the recording is written to.
     * @ghidraAddress NTSC-U/C: 0x00596290
     * @ghidraAddress PAL: 0x005d9698
     */
    explicit Recorder(OBStream *pStream);

    /**
     * Write one queued wrapper to the recording.
     *
     * The wrapper is saved and the stream's slot 2 is dispatched after it. Sch::Scheduler's delta
     * queueing path is the caller, for a recordable command while recording. The title is
     * inferred.
     *
     * @param pCommand The wrapper.
     * @ghidraAddress NTSC-U/C: 0x005962a0
     * @ghidraAddress PAL: 0x005d96a8
     */
    void Record(Sch::TimedCommand *pCommand);

private:
    OBStream *mStream; // +0x00
};

} // namespace Sch
