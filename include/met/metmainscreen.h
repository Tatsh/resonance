#pragma once

#include "met/metscreen.h"

class MetButtonList;

/**
 * Main menu.
 *
 * `13MetMainScreen` in the RTTI descriptor at `0x008f00e0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable at `0x007fb4f8` is the same length as the MetScreen table, and the
 * class declares no new virtual.
 *
 * New() allocates 0x90 bytes. The menu lists four buttons, tutorial, solo, multiplayer, and
 * options, and slot 36 opens the screen the chosen button leads to.
 *
 * The translation unit spans `0x002c60d0` to `0x002cb8c8`. Besides the members below, it has the
 * type function at `0x002cb488`, per-unit copies of MsgSink routines, and template library
 * emissions.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 19, 23, 24, 30, 33, 36, and 38.
 */
class MetMainScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * Supplies `fm` for the screen name, `metagame/_Solo` for the directory, and `2_main` for the
     * container, and allocates the button list.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002c60d0
     */
    MetMainScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list.
     *
     * @ghidraAddress 0x002cb570
     */
    virtual ~MetMainScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x002cb4e8
     */
    static MetMainScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Enter the menu, first saving the global settings when a configuration screen changed them.
     *
     * Slot 5. The renderer accepts commands from every pad up to index 4 again and the screen is
     * hidden. When MetFrontEndState::mUnknown18 and MetFrontEndState::mUnknown10 are both set,
     * the second is cleared and MetGlobalSettingsSaverScreen::StartSave() runs with this screen as
     * the one return screen. Otherwise EnterMenu() runs.
     *
     * @ghidraAddress 0x002c6dc0
     */
    virtual void EnterAndShow();

    /**
     * Step the selection, start the chosen button's alternation, or back out to the logo.
     *
     * Slot 19.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x002c6820
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002cb4d8
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x002cb4e0
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Exit forwards once the chosen button's alternation has finished.
     *
     * Slot 30.
     *
     * @param pObject The object whose alternation finished, which is not read.
     * @ghidraAddress 0x002c6c20
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Select the starting button, return the game to no mode, and post the help text.
     *
     * Slot 33. On arrival from MetLogoScreen, or with nothing selected, the return screen is
     * cleared and the solo button is selected once the tutorial is complete, the tutorial button
     * otherwise.
     *
     * @ghidraAddress 0x002c72a0
     */
    virtual void OnUnknownSlot33();

    /**
     * Bring up the next screen once this one has exited.
     *
     * Slot 36. After a back command the selection is cleared and MetLogoScreen returns. Otherwise
     * OpenSelectedButton() runs.
     *
     * @ghidraAddress 0x002c73e0
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the base views and add the four buttons with their labels and help texts.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x002c62a0
     */
    virtual void ResolveContainerViews();

private:
    // 0x002c7030. Promote a pending transition, bringing up the top logo, the small left gizmo, and
    // the help screen and activating this screen, then select the title preset and show the
    // screen. The title is inferred.
    void EnterMenu();

    // 0x002c7520. Open the screen the chosen button leads to. Solo goes to MetLoadPreFabScreen
    // while no save is possible, and otherwise to MetLoadFreqScreen or, with no saved identity,
    // MetLoadNewFreqScreen. The title is inferred.
    void OpenSelectedButton();

    MetButtonList *mButtonList; // +0x8c
};
