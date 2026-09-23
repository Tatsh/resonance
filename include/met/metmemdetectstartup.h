#pragma once

#include "met/fadeuser.h"
#include "met/metfade.h"
#include "met/metmemdetectscreen.h"

/**
 * First memory-card probe, run during start-up.
 *
 * `19MetMemDetectStartup` in the RTTI descriptor at `0x008eff90`, with two public non-virtual
 * bases at fixed offsets, MetMemDetectScreen at `+0x00` and FadeUser at `+160`. The object is 0xb0
 * bytes.
 *
 * Three vtables belong to the class, the 44-entry primary at `0x007fd518`, the four-entry FadeUser
 * table at `0x007fd440` that adjusts `this` by `-160`, and the 21-entry MemcardUser table at
 * `0x007fd468` that adjusts it by `-140`. The FadeUser table is where this class supplies the two
 * FadeUser pure virtuals. The primary is the same length as the MetMemDetectScreen table, and the
 * class declares no new virtual.
 *
 * The constructor at `0x002df058` takes only the renderer and the load priority. It runs the
 * MetMemDetectScreen constructor with an **empty** screen name, `metagame/Shared` for the
 * directory, and `memdetect1` for the container. The empty screen name is the one in the
 * subsystem, and it makes the two animation views resolve as `_EE.anim` and `_BF.anim` with
 * nothing before the underscore. MetLoadGameScreen uses an empty screen name as well. The
 * constructor then builds mFade, the same MetFade that MetLoadGameScreen builds.
 *
 * The destructor at `0x002e2eb8` releases mFade through the scalar deallocator with no null test,
 * which is what a delete expression compiles to for a class with no destructor, restores the
 * FadeUser vptr to `0x007ec070`, runs the MetMemDetectScreen destructor, and releases the object
 * with the tag `MsgSink`.
 *
 * Twelve slots differ from the MetMemDetectScreen table. Slots 20 through 25 sit eight bytes apart
 * at `0x002e2e00` through `0x002e2e28` and are two-instruction `jr ra` stubs. This screen plays
 * none of the six MetScreen sounds, the only class in the subsystem that silences all six.
 *
 * The screen shows over the start-up logo, fades out 360 frames after it enters, and probes the
 * card once the fade out has finished. A missing card raises `mem_check` 360 frames later. When
 * the probe ends, the screen fades back in and exits to MetSonyScreen.
 *
 * New() allocates 0xb0 bytes. The translation unit spans `0x002df058` to `0x002e33f0`. Besides the
 * members below, it has the type function at `0x002e2d80`, per-unit copies of MsgSink and FadeUser
 * routines, and template library emissions.
 */
class MetMemDetectStartup : public MetMemDetectScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002df058
     */
    MetMemDetectStartup(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002e2eb8
     */
    virtual ~MetMemDetectStartup();

    /**
     * Build the screen on the heap.
     *
     * MetScreen::CreateStartupScreens() registers this factory. It allocates 0xb0 bytes.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002e2e30
     */
    static MetMemDetectStartup *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Start the probe once the fade out has finished.
     *
     * FadeUser slot 2, through the FadeUser table entry that adjusts `this` by `-160`.
     *
     * @ghidraAddress 0x002e30f0
     */
    virtual void OnFadeOutDone();

    /**
     * Hide the screen and exit once the fade in has finished.
     *
     * FadeUser slot 3, through the FadeUser table entry that adjusts `this` by `-160`.
     *
     * @ghidraAddress 0x002e30a0
     */
    virtual void OnFadeInDone();

    /**
     * Attach the container view to the renderer, enable game drawing, hide the screen, and record
     * the time it entered.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x002e2f40
     */
    virtual void EnterAndShow();

    /**
     * Bring up MetSonyScreen.
     *
     * Slot 9. The override does not run the base exit.
     *
     * @ghidraAddress 0x002e2fb8
     */
    virtual void BeginExit();

    /**
     * Advance the fade, start the fade out 360 frames after the screen entered, raise `mem_check`
     * 360 frames after a missing card, and then run the base.
     *
     * Slot 26.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x002df250
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Show the detection notice and run the base probe.
     *
     * Slot 39.
     *
     * @ghidraAddress 0x002df6e0
     */
    virtual void StartDetect();

    /**
     * Record the time a missing card was found.
     *
     * Slot 41.
     *
     * @ghidraAddress 0x002e3058
     */
    virtual void OnNoCard();

    /**
     * Fade back in.
     *
     * Slot 42.
     *
     * @ghidraAddress 0x002e3068
     */
    virtual void OnDetectFinished();

    /**
     * Silence the slide sound.
     *
     * All six sound overrides are two-instruction stubs, each written inline with an empty body.
     * This is the one screen in the subsystem that silences every MetScreen sound.
     *
     * @ghidraAddress 0x002e2e00
     */
    virtual void PlaySlideSound(int) {
    }

    /**
     * Silence the leave sound.
     *
     * @ghidraAddress 0x002e2e08
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Silence the high sound.
     *
     * @ghidraAddress 0x002e2e10
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Silence the cycle-left sound.
     *
     * @ghidraAddress 0x002e2e18
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Silence the cycle-right sound.
     *
     * @ghidraAddress 0x002e2e20
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Silence the error sound.
     *
     * @ghidraAddress 0x002e2e28
     */
    virtual void PlayErrorSound(int) {
    }

private:
    MetFade *mFade; // +0xa4
    // The time OnNoCard() ran, or 0 once the `mem_check` dialogue is up. +0xa8
    float mNoCardTime;
    // The time EnterAndShow() ran, or 0 once the fade out has started. Not written by the
    // constructor. +0xac
    float mEnterTime;
};
