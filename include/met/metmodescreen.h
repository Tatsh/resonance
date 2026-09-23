#pragma once

#include "met/metscreen.h"

class MetButtonList;

namespace Rnd {
class Button;
} // namespace Rnd

/**
 * Screen that picks the game mode.
 *
 * `13MetModeScreen` in the RTTI descriptor at `0x008f00f0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007fe050`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * The constructor at `0x002e72c0` takes only the renderer and the load priority, and supplies
 * `smm` for the screen name, `metagame/Shared` for the directory, and `sm_mode` for the container.
 * It writes only `+0x8c`, into which it allocates a MetButtonList.
 *
 * The object is 0x90 bytes, the size New() allocates.
 *
 * The screen offers two buttons, the game and the jam. Selecting one sets the play mode and goes
 * on to the skill screen or the remix type screen. Backing out returns to the character picker in
 * a local multiplayer session, or to the freq or prefab loader in a solo one.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5 `0x002e79b8`, 19 `0x002e7628`, 23 `0x002eb808`, 24 `0x002eb810`, 30 `0x002eb958`, 33
 * `0x002eb920`, 36 `0x002e8008`, 38 `0x002e7490`.
 */
class MetModeScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002e72c0
     */
    MetModeScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list.
     *
     * @ghidraAddress 0x002eb8a0
     */
    virtual ~MetModeScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002eb818
     */
    static MetModeScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Select the button that matches the current play mode, fill the help prompts for a solo or a
     * multiplayer session, set the title, and enter the screen.
     *
     * Slot 5.
     *
     * @ghidraAddress 0x002e79b8
     */
    virtual void EnterAndShow();

    /**
     * Act on one navigation command.
     *
     * Slot 19. Previous and next walk the button ring and refresh the help prompt. Select clears
     * the panel and the prompt and starts the selected button's alternation. Back clears the prompt
     * and the panel, clears mUnknown18, exits the title and left gizmo screens, and begins the
     * exit.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002e7628
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Silence the cycle-left sound.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x002eb808
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Silence the cycle-right sound.
     *
     * @param nSelector Not read.
     * @ghidraAddress 0x002eb810
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Leave the screen once a button's alternation finishes.
     *
     * Slot 30. Stores 2 in mUnknown18, exits the title screen, and begins the exit.
     *
     * @param pButton The button that finished. Not read.
     * @ghidraAddress 0x002eb958
     */
    virtual void OnUnknownSlot30(Rnd::Button *pButton);

    /**
     * Refresh the help prompt for the selected button.
     *
     * Slot 33.
     *
     * @ghidraAddress 0x002eb920
     */
    virtual void OnUnknownSlot33();

    /**
     * Go on to the next screen once the exit finishes.
     *
     * Slot 36. A select, recorded by a non-zero mUnknown18, goes through GoToSelectedMode(). A back
     * returns to the screen the game mode calls for.
     *
     * @ghidraAddress 0x002e8008
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the container's views, then add the game and jam buttons.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x002e7490
     */
    virtual void ResolveContainerViews();

private:
    // 0x002e8398
    // Sets the play mode for the selected button and goes on to the skill screen for a game or the
    // remix type screen for a jam. The title is inferred.
    void GoToSelectedMode();

    MetButtonList *mUnknown8c; // +0x8c
};
