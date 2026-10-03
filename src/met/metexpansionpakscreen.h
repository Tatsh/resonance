#pragma once

#include "met/discswap.h"
#include "met/fadeuser.h"
#include "met/metfade.h"
#include "met/metscreen.h"

/**
 * Dialogue that swaps the game disc for an expansion disc of more songs and loads the expansion
 * disc archives.
 *
 * Its RTTI descriptor is at `0x008efdd0`. It has two public non-virtual bases at fixed offsets,
 * MetScreen at `+0x00`, and FadeUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x007ebf30`, the same length as the MetScreen table. The
 * class declares no new virtual. A diff against the MetScreen table at `0x0080b6a0` reads
 * ten overrides, slots 1, 5, 9, 15, 16, 23, 24, 26, and 38 apart from the type function.
 *
 * The four-entry FadeUser table at `0x007ebf08` adjusts `this` by `-140` in every entry, and its
 * slots 2 and 3 are at `0x0021a128` and `0x00219e80`. Neither address is in the primary table.
 *
 * The constructor at `0x00218320` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container. It writes `+0x8c`, which is the FadeUser vptr, clears mFade, and then allocates a
 * MetFade over the cleared pointer.
 *
 * The object is exactly 0xc4 bytes. Nothing derives from the class, so no base offset in any
 * descriptor pins the total, and the size comes instead from the `new` expression emitted out of
 * line at `0x0021d820`, which requests 0xc4 bytes from the tagged allocator under the tag
 * `MsgSink`. MsgSink is the base that declares the allocation operator, which is why the tag does
 * not identify this class.
 *
 * It is the second of only two classes that fill slot 16, the other being MetRemixDelScreen.
 *
 * The destructor is at `0x0021d8a8`.
 *
 * The screen is a fade-driven state machine. UpdateIdle() advances mState every frame through
 * mDiscSwap and mFade. Two message screens feed it, `expansion_prepare` and
 * `expansion_load`, whose appearance sets mPrepareShown and mLoadShown in OnMsgScreenShown().
 */
class MetExpansionPakScreen : public MetScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x00218320
     * @ghidraAddress PAL: 0x0022a848
     */
    MetExpansionPakScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress NTSC-U/C: 0x0021d8a8
     * @ghidraAddress PAL: 0x00230400
     */
    virtual ~MetExpansionPakScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x0021d820
     * @ghidraAddress PAL: 0x00230378
     */
    static MetExpansionPakScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Leave the screen once the fade out finishes.
     *
     * FadeUser slot 2, through the FadeUser table entry that adjusts `this`. The screen is erased
     * from the renderer's stack. A cancel returns to the options screens, and any other exit
     * returns to the main menu.
     *
     * @ghidraAddress NTSC-U/C: 0x0021a128
     * @ghidraAddress PAL: 0x0022cac0
     */
    virtual void OnFadeOutDone();

    /**
     * Show the `expansion_prepare` dialogue once the fade in finishes.
     *
     * FadeUser slot 3, through the FadeUser table entry that adjusts `this`. The dialogue has no
     * buttons.
     *
     * @ghidraAddress NTSC-U/C: 0x00219e80
     * @ghidraAddress PAL: 0x0022c7b8
     */
    virtual void OnFadeInDone();

    /**
     * Reset the state machine and start the fade in.
     *
     * Slot 5. mState through mMsgScreenExited are cleared, the record at `+0x90` is reset through
     * `0x0016a048`, and the fade starts on mFade over 360.0 units from MetRenderer::mAnimationFrame
     * with this screen's FadeUser subobject as the receiver. The MetScreen body does not run.
     *
     * @ghidraAddress NTSC-U/C: 0x0021d948
     * @ghidraAddress PAL: 0x002304a0
     */
    virtual void EnterAndShow();

    /**
     * Start the fade out.
     *
     * Slot 9. The same fade and the same 360.0 span as EnterAndShow(), through `0x0016a608`
     * rather than `0x0016d750`, and with a zero where EnterAndShow() passes a one. The MetScreen
     * body does not run, so the fade rather than the base drives the departure.
     *
     * @ghidraAddress NTSC-U/C: 0x0021d9a8
     * @ghidraAddress PAL: 0x00230500
     */
    virtual void BeginExit();

    /**
     * Advance the state machine once a message screen is dismissed.
     *
     * Slot 15. `expansion_prepare` shows `expansion_check` with two buttons, CONTINUE and CANCEL
     * the first time and RETRY and CANCEL once mRetry is set. `expansion_check` clears
     * mLoadShown and shows `expansion_load` whatever the choice. `expansion_done` records 2 in
     * MetScreen::mExitChoice and begins the exit, and any other name records 0 and begins it.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The response.
     * @ghidraAddress NTSC-U/C: 0x002193e8
     * @ghidraAddress PAL: 0x0022bb70
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Record which of the two message screens has appeared.
     *
     * Slot 16. `expansion_prepare` sets mPrepareShown and returns, and `expansion_load` sets
     * mLoadShown. Any other name is not recorded.
     *
     * @param name The message screen that appeared.
     * @ghidraAddress NTSC-U/C: 0x0021d9e0
     * @ghidraAddress PAL: 0x00230538
     */
    virtual void OnMsgScreenShown(const HxStr &name);

    /**
     * Silence the cycle-left sound.
     *
     * Slot 23. Both overrides are two-instruction stubs, so each was written inline with an empty
     * body.
     *
     * @ghidraAddress NTSC-U/C: 0x0021d810
     * @ghidraAddress PAL: 0x0022a290
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * Slot 24.
     *
     * @ghidraAddress NTSC-U/C: 0x0021d818
     * @ghidraAddress PAL: 0x0022a298
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Drive the dialogue state machine by one frame.
     *
     * Slot 26. mFade is ticked first, and the routine does nothing else once mFinished is set.
     * Otherwise mState advances through the record at `+0x90`. The open tray exits `MetMsgScreen`,
     * a disc found in the open tray shows `expansion_load`, a failed swap shows
     * `expansion_prepare` again, and a completed mount rebuilds the stage and arena lists,
     * merges the level lists of every identity and persona again, and shows `expansion_done`.
     *
     * @param flTime The current renderer time.
     * @ghidraAddress NTSC-U/C: 0x00218518
     * @ghidraAddress PAL: 0x0022aaa8
     */
    virtual void UpdateIdle(float flTime);

    /**
     * Resolve the container views.
     *
     * Slot 38. The override forwards to the MetScreen body with a direct call and does nothing
     * else. That is what the binary does. The slot is filled with a routine that adds no
     * behaviour.
     *
     * @ghidraAddress NTSC-U/C: 0x0021d928
     * @ghidraAddress PAL: 0x00230480
     */
    virtual void ResolveContainerViews();

private:
    // The disc exchange. EnterAndShow() resets it, and UpdateIdle() drives it and stores
    // each result as mState. +0x90
    DiscSwap mDiscSwap;
    // State of the dialogue. EnterAndShow() clears it and UpdateIdle() advances it through
    // the DiscSwap::Step values and its own 0, 1, 8, and 13. +0xa8
    int mState;
    // Set by OnMsgScreenShown() for `expansion_prepare`. +0xac
    int mPrepareShown;
    // Set by OnMsgScreenShown() for `expansion_load`. +0xb0
    int mLoadShown;
    // Set by UpdateIdle() once the mount completes. UpdateIdle() returns at once while it is set.
    // +0xb4
    int mFinished;
    // Set by UpdateIdle() after a failed swap. OnMsgScreenDismissed() then offers RETRY rather
    // than CONTINUE. +0xb8
    int mRetry;
    // Set by UpdateIdle() once it has exited `MetMsgScreen` at the open tray. +0xbc
    int mMsgScreenExited;
    // The fade both fade routines and the per-frame tick address. The constructor allocates it,
    // and the 0x2c-byte request and the constructor at `0x0016a1c8` are what identify the class.
    // The member spelling matches MetLoadGameScreen and MetMemDetectStartup, which build one the
    // same way. +0xc0
    MetFade *mFade;
};
