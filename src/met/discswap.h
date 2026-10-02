#pragma once

/**
 * Disc exchange that MetExpansionPakScreen runs to swap the game disc for the expansion disc.
 *
 * The record is 0x18 bytes, embedded in the screen at `+0x90`. It has no vtable, no RTTI, and no
 * literal of its own, so the title is inferred from what the routines do. They stop the drive and
 * open the tray, close the tray and wait for the drive, classify the inserted disc, and remount the
 * archives.
 *
 * Each polling routine returns the screen's next state, a Step value, and the screen stores the
 * result as its state without translating it. When the game is not reading from the disc alone,
 * the polling routines report their final step without touching the drive.
 */
class DiscSwap {
public:
    /** A screen state that a polling routine returns. The names are inferred. */
    enum Step {
        kStepEjecting = 2,    /*!< The drive is stopping or the tray is opening. */
        kStepEjectFailed = 3, /*!< The stop or the tray-open request was refused. */
        kStepTrayOpen = 4,    /*!< The tray is open, or the drive is not ready yet. */
        kStepClosing = 5,     /*!< The tray is closing, or the drive is spinning up. */
        kStepCloseFailed = 6, /*!< The tray-close request was refused. */
        kStepDiscReady = 7,   /*!< A PlayStation 2 disc is ready to mount. */
        kStepNotReady = 9,    /*!< The drive was not ready when the mount was attempted. */
        kStepMounted = 10,    /*!< The archives are mounted again. */
        kStepBadDisc = 11,    /*!< The disc is not a PlayStation 2 disc, or the mount failed. */
        kStepNoDisc = 12,     /*!< The tray closed with no disc in it. */
    };

    /**
     * Prepare a new exchange.
     *
     * Clears the poll count, sets the poll limit to 1800, records whether the game reads from the
     * disc alone, clears the no-disc flag, and caches configuration value 0x514.
     *
     * @ghidraAddress 0x0016a048
     */
    void Reset();

    /**
     * Let go of the current disc once the asynchronous reader is idle.
     *
     * While requests are still queued this pumps the finished ones and reports zero. Otherwise it
     * unmounts the session archives through CloseArk(). A call that unmounts them reports zero, so
     * the screen calls again on the next frame, and that call finds nothing mounted and reports
     * one. The record itself is not read.
     *
     * @return Non-zero when no archive was left to unmount.
     * @ghidraAddress 0x0016a098
     */
    int ReleaseDisc();

    /**
     * Restart the eject sequence.
     *
     * @ghidraAddress 0x0016a168
     */
    void BeginEject();

    /**
     * Advance the eject sequence by one poll.
     *
     * An empty drive skips the stop. Otherwise the drive is stopped, then the routine waits for the
     * drive to go idle and requests the tray open.
     *
     * @return kStepEjecting while the sequence runs, kStepEjectFailed when a request is refused,
     * and kStepTrayOpen once the tray is open.
     * @ghidraAddress 0x00169e50
     */
    int PollEject();

    /**
     * Restart the insert sequence.
     *
     * The body is identical to BeginEject(), but the two are separate routines in the image.
     *
     * @ghidraAddress 0x0016a170
     */
    void BeginInsert();

    /**
     * Advance the insert sequence by one poll.
     *
     * Waits for the drive to go idle, requests the tray closed, and then waits for the drive to
     * report ready. After the poll limit passes, the disc is classified anyway.
     *
     * @return kStepClosing while the sequence runs, kStepCloseFailed when the tray-close request
     * is refused, and the classification once the drive is ready.
     * @ghidraAddress 0x00169f08
     */
    int PollInsert();

    /**
     * Classify the disc once the drive is ready, without moving the tray.
     *
     * @return kStepTrayOpen while the drive is not ready, otherwise the classification.
     * @ghidraAddress 0x0016a0e8
     */
    int CheckDisc();

    /**
     * Mount the archives from the new disc and run script template 205.
     *
     * @return kStepNotReady when the drive is not ready, kStepBadDisc when the mount fails, and
     * kStepMounted otherwise.
     * @ghidraAddress 0x0016a178
     */
    int MountDisc();

private:
    // Sub-states of the eject and insert sequences. The values are the ones the image stores.
    enum Phase {
        kPhaseStart = 0,
        kPhaseStopping = 1,
        kPhaseOpenTray = 2,
        kPhaseOpened = 4,
        kPhaseCloseTray = 5,
        kPhaseSpinUp = 6,
        kPhaseClassify = 7,
    };

    // PollInsert() and CheckDisc() expand the same classification of sceCdGetDiskType().
    inline int ClassifyDisc();

    int phase_;
    int polls_;
    int pollLimit_;
    int discOnly_;    // Non-zero when GetHostMode() reports kHostModeCdOnly.
    int noDisc_;      // Set when the tray closed empty. The image does not read it back.
    int configValue_; // Configuration value 0x514. The image does not read it back.
};
