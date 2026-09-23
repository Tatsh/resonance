#pragma once

class GameManagerImpl;
class GameParams;
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
     * Open the recording file and start the watchdog's recording into it.
     *
     * Opens `rec.bin` through MakeFreqPath() as an OBFileStream and writes three length-prefixed
     * strings: `PS2 application.`, a description built from the level name, the game mode, the
     * play mode, and the GameParams word at `+0x20` (0 easy, 1 medium, otherwise hard), and
     * `no autoexec`. The manager then saves itself into the stream, and the watchdog records from
     * there on. GameManagerImpl::OnBeginGameLocal() is the caller. The title is inferred.
     *
     * @param nGameMode The manager's game mode.
     * @param params The manager's settings.
     * @ghidraAddress 0x0010c910
     */
    void BeginRecording(int nGameMode, const GameParams &params);

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
