#pragma once

class GameManagerImpl;
class GameParams;
class OBFileStream;

/**
 * Recording of a game session that GameManagerImpl::StartRecording() installs.
 *
 * The class is not polymorphic and emits no RTTI. EndRecordingCmd runs FinishUp() on it. The
 * object is 8 bytes and is allocated with the untagged allocator.
 */
class GameRecorder {
public:
    /**
     * @param pManager The manager that installs the recorder.
     * @ghidraAddress NTSC-U/C: 0x0010f010
     * @ghidraAddress PAL: 0x0010f470
     */
    explicit GameRecorder(GameManagerImpl *pManager);

    /**
     * Delete the stream, if any.
     *
     * @ghidraAddress NTSC-U/C: 0x0010f020
     * @ghidraAddress PAL: 0x0010f480
     */
    ~GameRecorder();

    /**
     * Open the recording file and start the watchdog's recording into it.
     *
     * Opens `rec.bin` through MakeFreqPath() as an OBFileStream and writes three length-prefixed
     * strings: a fixed banner beginning `PS2 application...`, a description built from the level
     * name, the game mode, the play mode, and GameParams::mDifficulty (0 easy, 1 medium, otherwise
     * hard), and `no autoexec`. The manager then saves itself into the stream, and the watchdog
     * records from there on. GameManagerImpl::OnBeginGameLocal() is the caller. The title is
     * inferred.
     *
     * @param nGameMode The manager's game mode.
     * @param params The manager's settings.
     * @ghidraAddress NTSC-U/C: 0x0010c910
     * @ghidraAddress PAL: 0x0010cae0
     */
    void BeginRecording(int nGameMode, const GameParams &params);

    /**
     * Close the watchdog's recording and delete the stream.
     *
     * EndRecordingCmd::Execute() is the caller.
     *
     * @ghidraAddress NTSC-U/C: 0x0010f080
     * @ghidraAddress PAL: 0x0010f4e0
     */
    void FinishUp();

    /**
     * Post an EndRecordingCmd for this recorder on the watchdog timer.
     *
     * The command runs as soon as the timer allows, under a fresh handle, and is recorded.
     * GameManagerImpl::EndGame() is the caller. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x0010cea0
     * @ghidraAddress PAL: 0x0010d178
     */
    void ScheduleEnd();

private:
    GameManagerImpl *mManager; // +0x00
    // Nothing recovered writes it apart from the constructor and FinishUp(), which clear it.
    OBFileStream *mStream; // +0x04
};
