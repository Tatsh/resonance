#pragma once

class GameManagerImpl;
class OBFileStream;

/**
 * Recording of a game session that GameManagerImpl::StartRecording() installs.
 *
 * The class is not polymorphic, emits no RTTI, and has no embedded file path, so the title is
 * inferred. EndRecordingCmd, whose name the RTTI attests, runs EndRecording() on it, which is the
 * evidence for the recording side. The object is 8 bytes and is allocated with the untagged
 * allocator.
 */
class GameRecorder {
public:
    /**
     * @param pManager The manager that installs the recorder.
     * @ghidraAddress 0x0010f010
     */
    explicit GameRecorder(GameManagerImpl *pManager);

    /**
     * Delete the stream, if any.
     *
     * @ghidraAddress 0x0010f020
     */
    ~GameRecorder();

    /**
     * Close the watchdog's recording and delete the stream.
     *
     * EndRecordingCmd::Execute() is the caller. The title is inferred.
     *
     * @ghidraAddress 0x0010f080
     */
    void EndRecording();

    /**
     * Post an EndRecordingCmd for this recorder on the watchdog timer.
     *
     * The command runs as soon as the timer allows, under a fresh handle, and is recorded.
     * GameManagerImpl::EndGame() is the caller. The title is inferred.
     *
     * @ghidraAddress 0x0010cea0
     */
    void ScheduleEnd();

private:
    GameManagerImpl *mManager; // +0x00
    // Nothing recovered writes it apart from the constructor and EndRecording(), which clear it.
    OBFileStream *mStream; // +0x04
};
