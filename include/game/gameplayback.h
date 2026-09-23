#pragma once

class GameManagerImpl;
class HxStr;

/**
 * Playback of a recorded game session that GameManagerImpl::StartPlayback() installs.
 *
 * The class is not polymorphic, emits no RTTI, and has no embedded file path, so the title is
 * inferred from its counterpart GameRecorder and from the manager slot that installs it. The object
 * is 4 bytes and is allocated with the untagged allocator.
 */
class GamePlayback {
public:
    /**
     * Restore the recorded session and start the watchdog's playback of it.
     *
     * Opens the recording as an IBFileStream on MakeFreqPath(file), reads three length-prefixed
     * strings from it and discards them, runs the manager's Load(), and hands the stream to
     * Watchdog::StartPlayback().
     *
     * @param file The recording.
     * @param pManager The manager that installs the playback.
     * @param nFlag The flag StartPlayback() passes. The body does not read it.
     * @ghidraAddress 0x0010cf30
     */
    GamePlayback(const HxStr &file, GameManagerImpl *pManager, int nFlag);

    /**
     * Close the watchdog's playback.
     *
     * @ghidraAddress 0x0010f0d8
     */
    ~GamePlayback();

private:
    GameManagerImpl *mManager; // +0x00
};
