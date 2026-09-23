#pragma once

#include "met/fadeuser.h"
#include "met/metscreen.h"

class MetFade;

/**
 * Publisher presentation screen shown at start-up.
 *
 * `13MetSonyScreen` in the RTTI descriptor at `0x008ef560`, with two public non-virtual bases at
 * fixed offsets, MetScreen at `+0x00`, and FadeUser at `+140`.
 *
 * The 39-entry primary vtable is at `0x0080f1e8`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The four-entry FadeUser table at `0x0080f1c0` adjusts `this` by `-140` in every entry.
 *
 * The constructor at `0x003ba0f8` takes only the renderer and the load priority, and supplies
 * `sony` for the screen name, `metagame/Shared` for the directory, and `sony_pres` for the
 * container. It writes `+0x8c`, which is the FadeUser vptr, clears the four members, and
 * allocates mFade.
 *
 * The object is 0xa0 bytes, the size New() allocates.
 *
 * The screen shows the publisher presentation for 480 frames after EnterAndShow(), fades out over
 * 360 frames, and once the fade out has finished waits a further 480 frames and for the arena,
 * the Freq Maker identities, and the end-game gizmo container before fading back in. Slot 36 then
 * plays the intro movie once and hands the renderer on to the rest of the front end.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 20 through 24, 26, 36, and 38, all declared below.
 */
class MetSonyScreen : public MetScreen, public FadeUser {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x003ba0f8
     */
    MetSonyScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x003bd7a8
     */
    virtual ~MetSonyScreen();

    /**
     * Build the screen on the heap.
     *
     * MetScreen::CreateStartupScreens() registers this factory.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x003bd720
     */
    static MetSonyScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Respond to a MetFade fade out having finished.
     *
     * FadeUser slot 2, through the FadeUser table entry that adjusts `this` by `-140`.
     *
     * @ghidraAddress 0x003bd908
     */
    virtual void OnFadeOutDone();

    /**
     * Respond to a MetFade fade in having finished.
     *
     * FadeUser slot 3, through the FadeUser table entry that adjusts `this` by `-140`.
     *
     * @ghidraAddress 0x003bd8b8
     */
    virtual void OnFadeInDone();

    /**
     * Play nothing. Slot 20.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x003bd6f8
     */
    virtual void PlaySlideSound(int nSelector);

    /**
     * Play nothing. Slot 21.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x003bd700
     */
    virtual void PlayLeaveSound(int nSelector);

    /**
     * Play nothing. Slot 22.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x003bd708
     */
    virtual void PlayHighSound(int nSelector);

    /**
     * Play nothing. Slot 23.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x003bd710
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing. Slot 24.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x003bd718
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Hide the screen and start the presentation clock. Slot 5.
     *
     * Records mHidden and the renderer's current frame in mEnterTime. The MetScreen body is not
     * run.
     *
     * @ghidraAddress 0x003bd868
     */
    virtual void EnterAndShow();

    /**
     * Step the fade and time the presentation. Slot 26.
     *
     * MetFade::Update() runs first. 480 frames after mEnterTime, the clock is cleared and a
     * 360-frame fade out starts, and a still hidden screen is shown. 480 frames after
     * mFadeOutDoneTime, once the arena load reports complete, the Freq Maker identities are
     * loaded, and `MetEndGameGizmoScreen` reports its container loaded, the clock is cleared and a
     * 360-frame fade in starts. The first time the fade in is due it is skipped once instead,
     * through a flag at `0x006cc138`.
     *
     * @param flTime The renderer's current frame.
     * @ghidraAddress 0x003ba2f0
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Play the intro movie once and hand the renderer on. Slot 36.
     *
     * Runs only while the flag at `0x006cc13c` is set, and clears it. When IntroMovieEnabled()
     * reports the movie wanted, the display memory is reset, the movie plays, and the display
     * mode is initialised again. MetFrontEndState's return screen then becomes this screen, and
     * Finish() runs.
     *
     * @ghidraAddress 0x003ba4c8
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container objects and hide the screen. Slot 38.
     *
     * @ghidraAddress 0x003bd828
     */
    virtual void ResolveContainerViews();

private:
    // 0x003ba620
    // Removes this screen from the renderer, clears its background scene, installs the main
    // loop's keep-alive draw as the bank-load progress hook, runs the synth's LoadBankSet4() and
    // the renderer's RendererBase slot 5, and hands the renderer a MetFreqEndedMsg with a payload
    // of 1. Slot 36 is its one caller, and the title is inferred.
    void Finish();

    // The frame EnterAndShow() ran at, cleared once the fade out starts. +0x90
    float mEnterTime;
    // The frame the fade out finished at, which OnFadeOutDone() records and slot 26 clears once
    // the fade in starts. +0x94
    float mFadeOutDoneTime;
    // The fade the screen drives. The constructor allocates it and the destructor releases it.
    MetFade *mFade; // +0x98
    // Set by EnterAndShow() while the screen is hidden, and cleared when slot 26 shows it. +0x9c
    int mHidden;
};
