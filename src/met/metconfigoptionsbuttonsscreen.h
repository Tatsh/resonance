#pragma once

#include "met/metbuttonlist.h"
#include "met/metscreen.h"

/**
 * Row of buttons along the network options screen.
 *
 * Its RTTI descriptor is at `0x008eeda8`. It has MetScreen as its one public non-virtual base at
 * offset 0. The object is 0x94 bytes and the 39-entry vtable is at `0x007ea688`, the same length as
 * the MetScreen table, and the class declares no new virtual.
 *
 * The screen loads `metagame/shared/net_options_butts.rnd` and resolves `net_options_butts.view`
 * along with the `nob_EE.anim` and `nob_BF.anim` animations. Its buttons open the game options,
 * the controller configuration, the memory card, and the credits, and a fifth button changes the
 * disc when GameOptions::mExpansionPack is set.
 *
 * Eight slots differ from the MetScreen table.
 *
 *  - 1 `0x0020c088` the destructor.
 *  - 5 `0x00207660` EnterAndShow().
 *  - 19 `0x002073c8` HandleCommand().
 *  - 23 `0x0020bff0` PlayCycleLeftSound(), overridden empty.
 *  - 24 `0x0020bff8` PlayCycleRightSound(), overridden empty.
 *  - 30 `0x00207fc0` OnRepeatingSoundFinished().
 *  - 36 `0x00208270` OnExitFinished().
 */
class MetConfigOptionsButtonsScreen : public MetScreen {
public:
    /**
     * Construct the button row.
     *
     * The screen name is `nob`, the directory `metagame/shared`, and the container
     * `net_options_butts`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002071f0
     */
    MetConfigOptionsButtonsScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0020c088
     */
    virtual ~MetConfigOptionsButtonsScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0020c000
     */
    static MetConfigOptionsButtonsScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Rebuild the buttons and their prompts, select the first, and enter.
     *
     * The disc button, its prompt, and the `nob_5group.view` frame appear only when
     * GameOptions::mExpansionPack is set, and `nob_4group.view` appears otherwise. The title is the
     * `config_option_buttons` title and the help layout is `standard_title`.
     *
     * @ghidraAddress 0x00207660
     */
    virtual void EnterAndShow();

    /**
     * Act on a command.
     *
     * Previous and next step the ring and post the new button's prompt. Select records the pad
     * of a controller button press in mControllerIndex, clears the active panel, and flashes the
     * selected button. Back records 0 in MetScreen::mExitChoice, exits the title and right gizmo
     * screens, and begins the exit.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002073c8
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x0020bff0
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x0020bff8
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Begin the exit once a button has finished flashing.
     *
     * Every button records 2 in MetScreen::mExitChoice and exits the right gizmo and title
     * screens. The memory, credits, and disc buttons also exit the help screen.
     *
     * @param pButton The button that finished alternating.
     * @ghidraAddress 0x00207fc0
     */
    virtual void OnRepeatingSoundFinished(Rnd::Button *pButton);

    /**
     * Push the screen the selected button opens.
     *
     * A cancel returns to the pause screen when MetFrontEndState::mReturnScreen is
     * `MetPauseSoloGameScreen`, and to the main menu otherwise. The controller button hands
     * mControllerIndex to MetConfigControllerScreen::mControllerIndex, and the memory button
     * records this screen in MetFrontEndState::mReturnScreen to return to.
     *
     * @ghidraAddress 0x00208270
     */
    virtual void OnExitFinished();

private:
    MetButtonList *mOptionButtons; // +0x8c
    // The controller index the controller configuration screen edits, the pad index of the
    // select command less one. +0x90
    int mControllerIndex;
};
