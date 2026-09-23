#pragma once

#include "met/discswap.h"
#include "met/fadeuser.h"
#include "met/metfade.h"
#include "met/metscreen.h"

/**
 * Dialogue shown when the expansion hardware is absent.
 *
 * `21MetExpansionPakScreen` in the RTTI descriptor at `0x008efdd0`, with two public non-virtual
 * bases at fixed offsets, MetScreen at `+0x00`, and FadeUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x007ebf30`, the same length as the MetScreen table, so the
 * class declares no virtual of its own. A diff against the MetScreen table at `0x0080b6a0` reads
 * ten overrides, slots 1, 5, 9, 15, 16, 23, 24, 26, and 38 apart from the type function.
 *
 * The four-entry FadeUser table at `0x007ebf08` adjusts `this` by `-140` in every entry, and its
 * slots 2 and 3 are at `0x0021a128` and `0x00219e80`. An earlier reading recorded both addresses
 * as primary slots 2 and 3. They are not in the primary table at all, and the table diff is what
 * settles that.
 *
 * The constructor at `0x00218320` takes only the renderer and the load priority, and supplies
 * `dlg` for the screen name, `metagame/Shared` for the directory, and `dialogue` for the
 * container. It writes `+0x8c`, which is the FadeUser vptr, clears mFade, and then allocates a
 * MetFade over the cleared pointer. An earlier reading of the routine stopped before that
 * allocation.
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
 * The screen is a fade-driven state machine. mUnknowna8 is the state, and OnUnknownSlot26() drives
 * it every frame through the embedded record at `+0x90` and through mFade. Two message screens
 * feed it, `expansion_prepare` and `expansion_load`, whose appearance sets mUnknownac and
 * mUnknownb0 in OnMsgScreenShown().
 */
class MetExpansionPakScreen : public MetScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00218320
     */
    MetExpansionPakScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0021d8a8
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
     * @ghidraAddress 0x0021d820
     */
    static MetExpansionPakScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Leave the screen once the fade out finishes.
     *
     * FadeUser slot 2, through the FadeUser table entry that adjusts `this`. The screen is erased
     * from the renderer's stack. A cancel returns to the options screens, and any other exit
     * returns to the main menu.
     *
     * @ghidraAddress 0x0021a128
     */
    virtual void OnFadeOutDone();

    /**
     * Show the `expansion_prepare` dialogue once the fade in finishes.
     *
     * FadeUser slot 3, through the FadeUser table entry that adjusts `this`. The dialogue has no
     * buttons.
     *
     * @ghidraAddress 0x00219e80
     */
    virtual void OnFadeInDone();

    /**
     * Reset the state machine and start the fade in.
     *
     * Slot 5. mUnknowna8 through mUnknownbc are cleared, the record at `+0x90` is reset through
     * `0x0016a048`, and the fade starts on mFade over 360.0 units from MetRenderer::mUnknown68
     * with this screen's FadeUser subobject as the receiver. The MetScreen body does not run.
     *
     * @ghidraAddress 0x0021d948
     */
    virtual void EnterAndShow();

    /**
     * Start the fade out.
     *
     * Slot 9. The same fade and the same 360.0 span as EnterAndShow(), through `0x0016a608`
     * rather than `0x0016d750`, and with a zero where EnterAndShow() passes a one. The MetScreen
     * body does not run, so the fade rather than the base drives the departure.
     *
     * @ghidraAddress 0x0021d9a8
     */
    virtual void BeginExit();

    /**
     * Advance the state machine once a message screen is dismissed.
     *
     * Slot 15. `expansion_prepare` shows `expansion_check` with two buttons, CONTINUE and CANCEL
     * the first time and RETRY and CANCEL once mUnknownb8 is set. `expansion_check` clears
     * mUnknownb0 and shows `expansion_load` whatever the choice. `expansion_done` records 2 in
     * MetScreen::mUnknown18 and begins the exit, and any other name records 0 and begins it.
     *
     * @param name The message screen that was dismissed.
     * @param nChoice The response.
     * @ghidraAddress 0x002193e8
     */
    virtual void OnMsgScreenDismissed(const HxStr &name, int nChoice);

    /**
     * Record which of the two message screens has appeared.
     *
     * Slot 16. `expansion_prepare` sets mUnknownac and returns, and `expansion_load` sets
     * mUnknownb0. A name matching neither records nothing.
     *
     * @param name The message screen that appeared.
     * @ghidraAddress 0x0021d9e0
     */
    virtual void OnMsgScreenShown(const HxStr &name);

    /**
     * Silence the cycle-left sound.
     *
     * Slot 23. Both overrides are two-instruction stubs, so each was written inline with an empty
     * body.
     *
     * @ghidraAddress 0x0021d810
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * Slot 24.
     *
     * @ghidraAddress 0x0021d818
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Drive the dialogue state machine by one frame.
     *
     * Slot 26. mFade is ticked first, and nothing else runs once mUnknownb4 is set. Otherwise
     * mUnknowna8 advances through the record at `+0x90`. The open tray exits `MetMsgScreen`, a
     * disc found in the open tray shows `expansion_load`, a failed swap shows
     * `expansion_prepare` again, and a completed mount rebuilds the stage and arena lists,
     * merges the level lists of every identity and persona again, and shows `expansion_done`.
     *
     * @param flTime The current renderer time.
     * @ghidraAddress 0x00218518
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Resolve the container views.
     *
     * Slot 38. The override forwards to the MetScreen body with a direct call and does nothing
     * else. That is what the binary does. The slot is filled with a routine that adds no
     * behaviour.
     *
     * @ghidraAddress 0x0021d928
     */
    virtual void ResolveContainerViews();

private:
    // The disc exchange. EnterAndShow() resets it, and OnUnknownSlot26() drives it and stores
    // each result as mUnknowna8. +0x90
    DiscSwap mUnknown90;
    // State of the dialogue. EnterAndShow() clears it and OnUnknownSlot26() advances it through
    // the DiscSwap::Step values and its own 0, 1, 8, and 13. +0xa8
    int mUnknowna8;
    // Set by OnMsgScreenShown() for `expansion_prepare`. +0xac
    int mUnknownac;
    // Set by OnMsgScreenShown() for `expansion_load`. +0xb0
    int mUnknownb0;
    // OnUnknownSlot26() returns at once while this is set. +0xb4
    int mUnknownb4;
    // Read by OnMsgScreenDismissed() to choose between its two branches. +0xb8
    int mUnknownb8;
    // Read by OnUnknownSlot26() at state 4. +0xbc
    int mUnknownbc;
    // The fade both fade routines and the per-frame tick address. The constructor allocates it,
    // and the 0x2c-byte request and the constructor at `0x0016a1c8` are what identify the class.
    // The member spelling matches MetLoadGameScreen and MetMemDetectStartup, which build one the
    // same way. +0xc0
    MetFade *mFade;
};
