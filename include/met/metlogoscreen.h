#pragma once

#include <vector>

#include "met/metscreen.h"

class Message;

namespace Rnd {
class Text;
class View;
} // namespace Rnd

/**
 * Panel that draws the game logo and waits for the player to start.
 *
 * `13MetLogoScreen` in the RTTI descriptor at `0x008efe00`, with MetScreen as its one public
 * non-virtual base at offset 0.
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
 */
class MetLogoScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * Supplies `fl` for the screen name, `metagame/Shared` for the directory, and
     * `freq_logo_panel` for the container, reads the attract delay from configuration code
     * 0x26c, and clears MetScreen::mUnknown60.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002ba4a0
     */
    MetLogoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x002be418
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
     * @ghidraAddress 0x002be390
     */
    static MetLogoScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Enter, and show the four legal texts.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x002be540
     */
    virtual void EnterAndShow();

    /**
     * Start the game on the select command or command 10.
     *
     * Slot 19. Plays `SND_MET_SLIDE`, clears MetRenderer::mUnknown60 and the blink, and exits.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002be4d8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * Slot 20. The body is empty.
     *
     * @ghidraAddress 0x002be368
     */
    virtual void PlaySlideSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 21. The body is empty.
     *
     * @ghidraAddress 0x002be370
     */
    virtual void PlayLeaveSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 22. The body is empty.
     *
     * @ghidraAddress 0x002be378
     */
    virtual void PlayHighSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @ghidraAddress 0x002be380
     */
    virtual void PlayCycleLeftSound(int) {
    }

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @ghidraAddress 0x002be388
     */
    virtual void PlayCycleRightSound(int) {
    }

    /**
     * Track the idle time for the attract mode, then blink the start text and play the wave.
     *
     * Slot 26. A controller press in the last poll restarts the idle count. Otherwise, when the
     * attract mode is enabled and has not started, the screen exits once the idle time passes the
     * configured delay.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x002bac40
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Blink the start text and play the wave.
     *
     * Slot 27.
     *
     * @param flTime The renderer's current animation frame position.
     * @ghidraAddress 0x002be5a8
     */
    virtual void UpdateIdleAnimation(float flTime);

    /**
     * Start the idle count and the blink, and play `SND_MET_FREQUENCY`, once the enter animation
     * has finished.
     *
     * Slot 33. Reads whether the attract mode is enabled from configuration code 0x26b and sets
     * MetRenderer::mUnknown60.
     *
     * @ghidraAddress 0x002bae40
     */
    virtual void OnUnknownSlot33();

    /**
     * Bring up the attract mode or the main menu once the screen has exited, and hide the legal
     * texts.
     *
     * Slot 36.
     *
     * @ghidraAddress 0x002baf20
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the animation views, the container view, the start text, the wave view, the version
     * text, and the four legal texts, and hide the screen.
     *
     * Slot 38. The base slot does not run, and the screen resolves MetScreen::mUnknown14 itself.
     *
     * @ghidraAddress 0x002ba6d0
     */
    virtual void ResolveContainerViews();

protected:
    /**
     * Record an unlock of the stages.
     *
     * Slot 3. A MetUnlockStagesMsg plays the activate sound and sets MetFrontEndState::mUnknown14.
     * Every other message is ignored.
     *
     * @param pMsg The message.
     * @ghidraAddress 0x002be6a0
     */
    virtual void HandleMessage(Message *pMsg);

private:
    // 0x002be670
    // Plays the activate sound and sets MetFrontEndState::mUnknown14. Slot 3 expands it, and the
    // address is its uncalled out-of-line copy. The title is inferred.
    static void RecordUnlock();

    // Toggles the start text every 120 frames and sets the wave view's frame. Slots 26 and 27 both
    // expand it. No out-of-line copy is emitted.
    void UpdateBlink(float flTime);

    Rnd::Text *mStartText;                // +0x8c
    float mBlinkTime;                     // +0x90, the time of the next toggle, or 0 while idle
    Rnd::View *mWaveView;                 // +0x94
    std::vector<Rnd::Text *> mLegalTexts; // +0x98
    int mUnknowna4;                       // +0xa4, not written by a recovered routine
    int mAttractEnabled;                  // +0xa8, from configuration code 0x26b
    int mAttractDelaySeconds;             // +0xac, from configuration code 0x26c
    int mAttractStarted;                  // +0xb0
    // The watchdog time of the last controller press, in nanoseconds. Starts at -1. +0xb8
    long long mLastActivityNs;
};
