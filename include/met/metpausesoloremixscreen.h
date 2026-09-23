#pragma once

#include "met/metpausebasescreen.h"

/**
 * Pause screen of a solo remix.
 *
 * `23MetPauseSoloRemixScreen` in the RTTI descriptor at `0x008ef550`, with MetPauseBaseScreen as
 * its one public non-virtual base at offset 0. The 40-entry vtable at `0x00803fc0` is the same
 * length as the MetPauseBaseScreen table, so the class declares no virtual of its own. New()
 * allocates 0xb4 bytes, four more than the base, for mUnknownb0.
 *
 * Beyond the base's commands, the screen can leave for the game options or the controller set-up.
 * It has no restart.
 *
 * The destructor at `0x00327ac0` is compiler-generated.
 */
class MetPauseSoloRemixScreen : public MetPauseBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `psr`, the directory `metagame/Transition`, and the container
     * `pause_soloremix`. The panel a dismissed confirmation reactivates is
     * `MetPauseSoloRemixScreen`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00323db8
     */
    MetPauseSoloRemixScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00327a38
     */
    static MetPauseSoloRemixScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Fill the paused heading and the option labels, then enter.
     *
     * Slot 5. The heading `psr_paused.txt` reads configuration code 0x258 under `pause_remix`, and
     * the labels come from code 0x259 under `pause_solo_remix`. Every label is copied to the
     * option text at the same index before MetPauseBaseScreen::EnterAndShow() copies them again,
     * and mUnknownb0 is cleared afterwards.
     *
     * @ghidraAddress 0x00324260
     */
    virtual void EnterAndShow();

    /**
     * Leave for the game options on code 7 or the controller set-up on code 8.
     *
     * Slot 19. Either one sets mUnknownb0, plays the bank's slide sound, clears the active panel,
     * and begins the exit. A back and code 10 go to MetPauseBaseScreen::HandleCommand(). Every
     * other code, a select among them, is ignored.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x003240a8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Open the configuration screen the command chose once the exit finishes.
     *
     * Slot 36. Without mUnknownb0 set, MetPauseBaseScreen::OnUnknownSlot36() runs instead. The
     * return screen in MetFrontEndState::mUnknown24 becomes `MetPauseSoloRemixScreen`, and
     * `MetHelpScreen`, `MetScreenTitleScreen`, and the configuration screen are pushed before the
     * configuration screen is made the active panel.
     *
     * @ghidraAddress 0x00324580
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views and the four option texts `psr_opt1.txt` to `psr_opt4.txt`.
     *
     * Slot 38. MetScreen::ResolveContainerViews() runs first. No text is tested for null.
     *
     * @ghidraAddress 0x00323f58
     */
    virtual void ResolveContainerViews();

private:
    // Set when the exit leads to a configuration screen rather than back to the game.
    int mUnknownb0;
};
