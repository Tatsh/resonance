#pragma once

#include "met/metpausebasescreen.h"

/**
 * Pause screen of a solo game.
 *
 * `22MetPauseSoloGameScreen` in the RTTI descriptor at `0x008ef540`, with MetPauseBaseScreen as its
 * one public non-virtual base at offset 0. The 40-entry vtable at `0x00803918` is the same length
 * as the MetPauseBaseScreen table, so the class declares no virtual of its own. New() allocates
 * 0xb4 bytes, four more than the base, for mUnknownb0.
 *
 * Beyond the base's commands, the screen can leave for the game options or the controller set-up.
 *
 * The destructor at `0x00323ae8` is compiler-generated.
 */
class MetPauseSoloGameScreen : public MetPauseBaseScreen {
public:
    /**
     * Construct the screen.
     *
     * The screen name is `psgr`, the directory `metagame/Transition`, and the container
     * `pause_solo`. The panel a dismissed confirmation reactivates is `MetPauseSoloGameScreen`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x0031fd98
     */
    MetPauseSoloGameScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00323a60
     */
    static MetPauseSoloGameScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Fill the paused heading and the five option labels, then enter.
     *
     * Slot 5. The heading `psgr_paused.txt` reads configuration code 0x258 under `pause_remix` in
     * jam mode and `pause_game` otherwise. The labels come from code 0x259 under
     * `pause_solo_game` in game mode and `pause_solo_remix` otherwise. The first five labels are
     * copied to the option texts before MetPauseBaseScreen::EnterAndShow() copies them again, and
     * mUnknownb0 is cleared afterwards.
     *
     * @ghidraAddress 0x00320230
     */
    virtual void EnterAndShow();

    /**
     * Leave for the game options on code 7 or the controller set-up on code 8.
     *
     * Slot 19. Either one sets mUnknownb0, plays the bank's slide sound, clears the active panel,
     * and begins the exit. A select, a back, and code 10 go to MetPauseBaseScreen::HandleCommand().
     * Code 9 is ignored.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00320088
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Open the configuration screen the command chose once the exit finishes.
     *
     * Slot 36. Without mUnknownb0 set, MetPauseBaseScreen::OnUnknownSlot36() runs instead. The
     * return screen in MetFrontEndState::mUnknown24 becomes `MetPauseSoloGameScreen`, and
     * `MetScreenTitleScreen`, the configuration screen, and `MetHelpScreen` are pushed before the
     * configuration screen is made the active panel.
     *
     * @ghidraAddress 0x003205a8
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container views and the five option texts `psgr_opt1.txt` to `psgr_opt5.txt`.
     *
     * Slot 38. MetScreen::ResolveContainerViews() runs first. No text is tested for null.
     *
     * @ghidraAddress 0x0031ff38
     */
    virtual void ResolveContainerViews();

private:
    // Set when the exit leads to a configuration screen rather than back to the game.
    int mUnknownb0;
};
