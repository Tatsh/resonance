#pragma once

#include <vector>

#ifdef VIDEO_STANDARD_PAL
#include "met/fadeuser.h"
#include "met/metfade.h"
#endif
#include "met/metscreen.h"

class Message;

namespace Rnd {
class Text;
class View;
} // namespace Rnd

/**
 * Panel that draws the game logo and waits for the player to start.
 *
 * Its RTTI descriptor is at `0x008efe00`. It has MetScreen as its one public non-virtual base at
 * offset 0.
 *
 * The 39-entry primary vtable is at `0x007fa208`, the same length as the MetScreen table, and the
 * class declares no new virtual. It and MetMsgScreen are the only two classes that override slot
 * 3 with a body rather than inheriting the empty MetScreen override. It is also the only class
 * that overrides slot 27.
 *
 * The object is 0xc0 bytes, the size New() allocates.
 *
 * The screen blinks `start text.txt` every 120 frames and plays `wave.view`. The select command
 * moves on to MetMainScreen. When the configuration enables it, the screen also counts the time
 * since the last controller press and, once the configured delay passes, exits to the attract
 * mode run by MetLoadGameScreen.
 *
 * The translation unit spans `0x002ba4a0` to `0x002be968`. Besides the members below, it has the
 * type function at `0x002be318`, a per-unit copy of MsgSink::HandleDefault, and template library
 * emissions.
 *
 * The European release adds FadeUser as a second base at `+0x8c`. Every member before
 * mLastActivityNs sits 4 bytes later, and mFade follows at `+0xc0`. The object is 0xc8 bytes. An
 * exit with MetScreen::mExitChoice at 0 fades the screen in over 360 frames and removes it from the
 * renderer when the fade finishes.
 */
#ifdef VIDEO_STANDARD_PAL
class MetLogoScreen : public MetScreen, public FadeUser {
#else
class MetLogoScreen : public MetScreen {
#endif
public:
    /**
     * Construct the screen.
     *
     * Supplies `fl` for the screen name, `metagame/Shared` for the directory, and
     * `freq_logo_panel` for the container, reads the attract delay from configuration code
     * 0x26c, and clears MetScreen::mShowsLoadedDrawables. The European release then builds mFade.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress NTSC-U/C: 0x002ba4a0
     * @ghidraAddress PAL: 0x002d9c10
     */
    MetLogoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Release the screen. The European release first releases mFade without a null test.
     *
     * @ghidraAddress NTSC-U/C: 0x002be418
     * @ghidraAddress PAL: 0x002da698
     */
    virtual ~MetLogoScreen();

    /**
     * Build the screen on the heap.
     *
     * MetScreen::CreateStartupScreens() registers this factory.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress NTSC-U/C: 0x002be390
     * @ghidraAddress PAL: 0x002de138
     */
    static MetLogoScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Enter, and show the four legal texts.
     *
     * Slot 5.
     *
     * @ghidraAddress NTSC-U/C: 0x002be540
     * @ghidraAddress PAL: 0x002de248
     */
    virtual void EnterAndShow();

    /**
     * Start the game on the select command or command 10.
     *
     * Slot 19. Plays `SND_MET_SLIDE`, clears MetRenderer::mTitlePromptShowing and the blink, and
     * exits. The European release also sets MetScreen::mExitChoice to 2.
     *
     * @param pCommand The command.
     * @ghidraAddress NTSC-U/C: 0x002be4d8
     * @ghidraAddress PAL: 0x002de1c0
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * Slot 20. The body is empty.
     *
     * @ghidraAddress NTSC-U/C: 0x002be368
     * @ghidraAddress PAL: 0x002de108
     */
    virtual void PlaySlideSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 21. The body is empty.
     *
     * @ghidraAddress NTSC-U/C: 0x002be370
     * @ghidraAddress PAL: 0x002de110
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 22. The body is empty.
     *
     * @ghidraAddress NTSC-U/C: 0x002be378
     * @ghidraAddress PAL: 0x002de118
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @ghidraAddress NTSC-U/C: 0x002be380
     * @ghidraAddress PAL: 0x002de120
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @ghidraAddress NTSC-U/C: 0x002be388
     * @ghidraAddress PAL: 0x002de128
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Track the idle time for the attract mode, then blink the start text and play the wave.
     *
     * Slot 26. A controller press in the last poll restarts the idle count. Otherwise, when the
     * attract mode is enabled and has not started, the screen exits once the idle time passes the
     * configured delay. The European release then advances mFade.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress NTSC-U/C: 0x002bac40
     * @ghidraAddress PAL: 0x002da778
     */
    virtual void UpdateIdle(float flTime);

    /**
     * Blink the start text and play the wave.
     *
     * Slot 27.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress NTSC-U/C: 0x002be5a8
     * @ghidraAddress PAL: 0x002de2b0
     */
    virtual void UpdateIdleAnimation(float flTime);

    /**
     * Start the idle count and the blink, and play `SND_MET_FREQUENCY`, once the enter animation
     * has finished.
     *
     * Slot 33. Reads whether the attract mode is enabled from configuration code 0x26b and sets
     * MetRenderer::mTitlePromptShowing.
     *
     * @ghidraAddress NTSC-U/C: 0x002bae40
     * @ghidraAddress PAL: 0x002da988
     */
    virtual void OnEnterFinished();

    /**
     * Bring up the attract mode or the main menu once the screen has exited, and hide the legal
     * texts.
     *
     * Slot 36. In the European release the main menu needs MetScreen::mExitChoice at 2. At 0 the
     * screen fades in through mFade and adds itself to the renderer, and any other value only hides
     * the legal texts.
     *
     * @ghidraAddress NTSC-U/C: 0x002baf20
     * @ghidraAddress PAL: 0x002daa68
     */
    virtual void OnExitFinished();

    /**
     * Resolve the animation views, the container view, the start text, the wave view, the version
     * text, and the four legal texts, and hide the screen.
     *
     * Slot 38. The base slot does not run, and the screen resolves MetScreen::mView itself. The
     * European release sets the start text and the four legal texts to their texts in the current
     * language.
     *
     * @ghidraAddress NTSC-U/C: 0x002ba6d0
     * @ghidraAddress PAL: 0x002d9f00
     */
    virtual void ResolveContainerViews();

protected:
    /**
     * Record an unlock of the stages.
     *
     * Slot 3. A MetUnlockStagesMsg plays the activate sound and sets MetFrontEndState::mUnlockAll.
     * Every other message is ignored.
     *
     * @param pMsg The message.
     * @ghidraAddress NTSC-U/C: 0x002be6a0
     * @ghidraAddress PAL: 0x002de3c8
     */
    virtual void DispatchPriv(Message *pMsg);

#ifdef VIDEO_STANDARD_PAL
    /**
     * Do nothing.
     *
     * FadeUser slot. The body is empty.
     *
     * @ghidraAddress PAL: 0x002de130
     */
    virtual void OnFadeOutDone() {
    }

    /**
     * Remove this screen from the renderer once the fade in has finished.
     *
     * FadeUser slot.
     *
     * @ghidraAddress PAL: 0x002de3a8
     */
    virtual void OnFadeInDone();
#endif

private:
    /**
     * Plays the activate sound and sets MetFrontEndState::mUnlockAll.
     *
     * Slot 3 expands it, and the address is its uncalled out-of-line copy. The title is inferred.
     *
     * @ghidraAddress NTSC-U/C: 0x002be670
     * @ghidraAddress PAL: 0x002de378
     */
    static void RecordUnlock();

    // Toggles the start text every 120 frames and sets the wave view's frame. Slots 26 and 27 both
    // expand it. No out-of-line copy is emitted.
    void UpdateBlink(float flTime);

    Rnd::Text *mStartText;                // +0x8c
    float mBlinkTime;                     // +0x90, the time of the next toggle, or 0 while idle
    Rnd::View *mWaveView;                 // +0x94
    std::vector<Rnd::Text *> mLegalTexts; // +0x98
    int mReserved;                        // +0xa4, never read or written
    int mAttractEnabled;                  // +0xa8, from configuration code 0x26b
    int mAttractDelaySeconds;             // +0xac, from configuration code 0x26c
    int mAttractStarted;                  // +0xb0
    // The watchdog time of the last controller press, in nanoseconds. Starts at -1. +0xb8
    long long mLastActivityNs;
#ifdef VIDEO_STANDARD_PAL
    // The fade that OnExitFinished() runs with mExitChoice at 0. +0xc0
    MetFade *mFade;
#endif
};
