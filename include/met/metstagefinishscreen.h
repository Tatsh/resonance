#pragma once

#include "met/metscreen.h"

/**
 * Congratulation screen shown when a stage is finished.
 *
 * `20MetStageFinishScreen` in the RTTI descriptor at `0x008ef8e0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x0080f9e0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x003bdbb8` takes only the renderer and the load priority, and supplies
 * `egc` for the screen name, `metagame/_Solo` for the directory, and `end_game_congrats` for the
 * container. It writes two vectors at `+0x8c` and `+0x98`, then `+0xa4`, `+0xa8`, `+0xac`,
 * `+0xb0`, and `+0xb4`.
 *
 * The object is at least 0xb8 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the figure is the lower bound the constructor's highest store
 * gives.
 *
 * The destructor is at `0x003bded8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003be2a8`, 19 `0x003bf788`, 21 `0x003c4228`, 22 `0x003c4230`, 23 `0x003c4238`, 24
 * `0x003c4240`, 25 `0x003c4248`, 26 `0x003bf5e8`, 30 `0x003c4390`, 33 `0x003c42d8`, 36
 * `0x003c0530`, 38 `0x003be068`.
 */
class MetStageFinishScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003bdbb8
     */
    MetStageFinishScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003bded8
     */
    virtual ~MetStageFinishScreen();

    /**
     * @ghidraAddress 0x003c4228
     */
    virtual void PlayLeaveSound();

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003c4230
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003c4238
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003c4240
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x003c4248
     */
    virtual void PlayErrorSound(int nSelector);

private:
    /**
     * Queue the message for a difficulty unlocked by this stage.
     *
     * Slot 5 at `0x003be4ac` is the one caller. The body clears the word at `+0xb4`, and acts only
     * when the unlock is new: it reads the game manager's GameParams, the level's stage
     * (configuration code 0x25d), and the difficulty name from DifficultyName(), and queues the
     * matching configuration string (code 0x258, keys such as `end_game_easy_normal`) on the vector
     * of messages at `+0x8c`. The body is not written, because the class declares no members yet.
     *
     * @param nWasUnlocked Non-zero when the difficulty was already unlocked.
     * @param nIsUnlocked Non-zero when the difficulty is unlocked now.
     * @ghidraAddress 0x003c0008
     */
    void AddDifficultyUnlockMessage(int nWasUnlocked, int nIsUnlocked);

    /**
     * Queue `end_game_secret` when the secret unlock is new.
     *
     * Appends configuration string 0x258 for the key to the vector of messages at `+0x8c` when
     * nIsUnlocked is set and nWasUnlocked is clear. Slot 5 at `0x003be4c4` is the one caller. The
     * body is not written, for the reason AddDifficultyUnlockMessage() gives.
     *
     * @param nWasUnlocked Non-zero when the unlock already existed.
     * @param nIsUnlocked Non-zero when it exists now.
     * @ghidraAddress 0x003c02c0
     */
    void AddSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked);

    /**
     * Queue `end_game_super_secret` when that unlock is new.
     *
     * The same shape as AddSecretUnlockMessage(). Slot 5 at `0x003be4dc` is the one caller.
     *
     * @param nWasUnlocked Non-zero when the unlock already existed.
     * @param nIsUnlocked Non-zero when it exists now.
     * @ghidraAddress 0x003c0390
     */
    void AddSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked);

    /**
     * Queue `end_game_end_super_secret` when that unlock is new.
     *
     * The same shape as AddSecretUnlockMessage(). Slot 5 at `0x003be700` is the one caller.
     *
     * @param nWasUnlocked Non-zero when the unlock already existed.
     * @param nIsUnlocked Non-zero when it exists now.
     * @ghidraAddress 0x003c0460
     */
    void AddEndSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked);
};
