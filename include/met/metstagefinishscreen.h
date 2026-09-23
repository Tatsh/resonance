#pragma once

#include <vector>

#include "met/metscreen.h"
#include "os/hxstr.h"

class MetButtonList;

namespace Rnd {
class Drawable;
class Object;
} // namespace Rnd

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
 * container.
 *
 * The object is 0xb8 bytes, which the allocation at `0x003c4250` fixes. That routine allocates
 * under the tag `MsgSink`, runs the constructor, and returns the object, which is what a `new`
 * expression compiles to. Like the one at `0x003b9b98` for MetSoloWinScreen, its one caller is
 * the routine at `0x00385180` that creates every front-end screen, and it is not declared.
 *
 * The destructor is at `0x003bded8`.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x003be2a8`, 19 `0x003bf788`, 21 `0x003c4228`, 22 `0x003c4230`, 23 `0x003c4238`, 24
 * `0x003c4240`, 25 `0x003c4248`, 26 `0x003bf5e8`, 30 `0x003c4390`, 33 `0x003c42d8`, 36
 * `0x003c0530`, 38 `0x003be068`.
 *
 * Slot 5 compares the campaign statistics before and after recording the finished stage, and
 * queues one message for each change through the private builders. Slot 26 then shows the queued
 * messages one at a time, 360 frames apart, and shows the continue button after the last.
 *
 * Slot 5, slot 36, the message layout routine at `0x003bef98`, the stage-complete builder at
 * `0x003bfc48`, and AddDifficultyUnlockMessage() are not written yet. Each reads the object the
 * unrecovered singleton getter at `0x00217f30` vends.
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
     * Start the continue button alternating once the messages are done. Slot 19.
     *
     * Every command is discarded during the message sequence, and every command other than
     * kMetScreenCommandSelect is discarded afterwards, so the screen cannot be departed with the
     * back button.
     *
     * @param pCommand The command to route.
     * @ghidraAddress 0x003bf788
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play no leave sound. Slot 21.
     *
     * @ghidraAddress 0x003c4228
     */
    virtual void PlayLeaveSound();

    /**
     * Play no high sound. Slot 22.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x003c4230
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play no left cycle sound. Slot 23.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x003c4238
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play no right cycle sound. Slot 24.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x003c4240
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Play no error sound. Slot 25.
     *
     * @param nSelector The controller index, which the body does not read.
     * @ghidraAddress 0x003c4248
     */
    virtual void PlayErrorSound(int nSelector);

    /**
     * Show the next queued message, or the continue button after the last. Slot 26.
     *
     * The slot acts only inside the message sequence slot 33 starts, and only once 360 frames have
     * passed since the last step. It shows the drawable of the next message when one remains and
     * advances the index. Once the
     * index passes the message count it shows and selects the continue button, clears the time the
     * message appeared, and activates the panel `MetStageFinishScreen`; otherwise it records the
     * current time. The test is against the index after it advances, so the button appears one
     * interval after the last message rather than with it.
     *
     * @param flTime The current frame position.
     * @ghidraAddress 0x003bf5e8
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Start this screen's exit animation. Slot 30.
     *
     * @param pObject The object slot 29 finished with, which the body does not read.
     * @ghidraAddress 0x003c4390
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Select no button, record the time the first message appears, and clear the panel. Slot 33.
     *
     * MetScreen slot 32 runs the slot once the enter animation has finished.
     *
     * @ghidraAddress 0x003c42d8
     */
    virtual void OnUnknownSlot33();

    /**
     * Resolve the base views, add the continue button, and fill the four congratulation texts.
     * Slot 38.
     *
     * Each of `egc_congrat1.txt` through `egc_congrat4.txt` receives the configuration string for
     * `end_game_congrats`. The text is not tested for null after the lookup.
     *
     * @ghidraAddress 0x003be068
     */
    virtual void ResolveContainerViews();

private:
    /**
     * Queue `end_game_high_score` when the stage score beats a recorded high score.
     *
     * Slot 5 at `0x003be440` is the one caller. A previous score of zero queues nothing.
     *
     * @param nPreviousScore The high score recorded before this game.
     * @param nScore The score of this game.
     * @ghidraAddress 0x003bf8e8
     */
    void AddHighScoreMessage(int nPreviousScore, int nScore);

    /**
     * Queue `end_game_arena_complete` for an arena finished for the first time.
     *
     * The configuration string is a format that receives the display name (configuration code
     * 0x326) of the arena at index nCompleted - 1 in GetArenaList(). Slot 5 at `0x003be450` is the
     * one caller.
     *
     * @param nPreviousCompleted The number of completed arenas before this game.
     * @param nCompleted The number of completed arenas now.
     * @ghidraAddress 0x003bf9b8
     */
    void AddArenaCompleteMessage(int nPreviousCompleted, int nCompleted);

    /**
     * Queue `stage_score_beat` when the stage's target score is beaten for the first time.
     *
     * Slot 5 at `0x003be470` is the one caller.
     *
     * @param nWasBeaten Non-zero when the target was already beaten.
     * @param nIsBeaten Non-zero when the target is beaten now.
     * @ghidraAddress 0x003bfb78
     */
    void AddStageScoreBeatMessage(int nWasBeaten, int nIsBeaten);

    /**
     * Queue the message for a difficulty unlocked by this stage.
     *
     * Slot 5 at `0x003be4ac` is the one caller. The body clears mUnknownb4, and acts only when the
     * unlock is new. It reads the game manager's GameParams, the level's stage (configuration code
     * 0x25d), and the difficulty name from DifficultyName(), and queues the matching configuration
     * string (code 0x258, keys such as `end_game_easy_normal`) on mUnknown8c. The body is not
     * written, for the reason the class documentation records.
     *
     * @param nWasUnlocked Non-zero when the difficulty was already unlocked.
     * @param nIsUnlocked Non-zero when the difficulty is unlocked now.
     * @ghidraAddress 0x003c0008
     */
    void AddDifficultyUnlockMessage(int nWasUnlocked, int nIsUnlocked);

    /**
     * Queue `end_game_secret` when the secret unlock is new.
     *
     * Slot 5 at `0x003be4c4` is the one caller.
     *
     * @param nWasUnlocked Non-zero when the unlock already existed.
     * @param nIsUnlocked Non-zero when it exists now.
     * @ghidraAddress 0x003c02c0
     */
    void AddSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked);

    /**
     * Queue `end_game_super_secret` when that unlock is new.
     *
     * Slot 5 at `0x003be4dc` is the one caller.
     *
     * @param nWasUnlocked Non-zero when the unlock already existed.
     * @param nIsUnlocked Non-zero when it exists now.
     * @ghidraAddress 0x003c0390
     */
    void AddSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked);

    /**
     * Queue `end_game_end_super_secret` when that unlock is new.
     *
     * Slot 5 at `0x003be700` is the one caller.
     *
     * @param nWasUnlocked Non-zero when the unlock already existed.
     * @param nIsUnlocked Non-zero when it exists now.
     * @ghidraAddress 0x003c0460
     */
    void AddEndSuperSecretUnlockMessage(int nWasUnlocked, int nIsUnlocked);

    // The messages slot 26 shows, in the order slot 5 queued them. +0x8c
    std::vector<HxStr> mUnknown8c;
    // One drawable per message, which slot 26 shows. The routine at 0x003bef98 fills it. +0x98
    std::vector<Rnd::Drawable *> mUnknown98;
    // The one-button ring with the continue button. The constructor allocates it and the
    // destructor releases it. +0xa4
    MetButtonList *mUnknowna4;
    // Set by slot 5 once the stage is recorded, which blocks a second recording, and cleared by
    // slot 36. +0xa8
    int mUnknowna8;
    // Index of the next message slot 26 shows. +0xac
    int mUnknownac;
    // Frame position of the last step of the message sequence, or zero outside the sequence. +0xb0
    float mUnknownb0;
    // Cleared by AddDifficultyUnlockMessage() and handed to MetSoloWinScreen by slot 36. +0xb4
    int mUnknownb4;
};
