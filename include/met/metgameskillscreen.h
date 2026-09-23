#pragma once

#include "met/metscreen.h"

class MetButtonList;

/**
 * Screen that picks the difficulty.
 *
 * `18MetGameSkillScreen` in the RTTI descriptor at `0x008ef8b0`, with MetScreen as its one public
 * non-virtual base at offset 0.
 *
 * The 39-entry primary vtable is at `0x007f38b0`, the same length as the MetScreen table, so the
 * class declares no virtual of its own.
 *
 * New() allocates 0x90 bytes. The screen lists three buttons, easy, normal, and expert, and the
 * selected index becomes GameParams::mDifficulty when the screen is left forwards.
 *
 * The translation unit spans `0x00273140` to `0x00276d38`. Besides the members below, it has the
 * type function at `0x002769e8`, per-unit copies of MsgSink routines, and template library
 * emissions.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 5, 19, 23, 24, 30, 36, and 38.
 */
class MetGameSkillScreen : public MetScreen {
public:
    /**
     * Construct the screen.
     *
     * Supplies `smgs` for the screen name, `metagame/_Solo` for the directory, and `gameskill` for
     * the container, and allocates the button list.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00273140
     */
    MetGameSkillScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Delete the button list.
     *
     * @ghidraAddress 0x00276ad0
     */
    virtual ~MetGameSkillScreen();

    /**
     * Build the screen on the heap.
     *
     * The routine at `0x00385180` that creates every front-end screen is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x00276a48
     */
    static MetGameSkillScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Select the current difficulty, fill the help texts and the title for the game mode, and
     * show the screen.
     *
     * Slot 5. The solo mode reads the `solo` title prefix and the `smgs_` help texts, and every
     * other mode reads the `multi` prefix and the `mgs_` help texts.
     *
     * @ghidraAddress 0x00273800
     */
    virtual void EnterAndShow();

    /**
     * Step the selection, start the chosen button's alternation, or back out to the title screen.
     *
     * Slot 19.
     *
     * @param pCommand The command.
     * @ghidraAddress 0x00273558
     */
    virtual void HandleCommand(const MetScreenCommand *pCommand);

    /**
     * Play nothing.
     *
     * Slot 23. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00276a38
     */
    virtual void PlayCycleLeftSound(int nSelector);

    /**
     * Play nothing.
     *
     * Slot 24. The body is empty.
     *
     * @param nSelector The value the override compares against its own recorded selector.
     * @ghidraAddress 0x00276a40
     */
    virtual void PlayCycleRightSound(int nSelector);

    /**
     * Leave forwards once the chosen button's alternation has finished, exiting the left gizmo
     * and the title screens.
     *
     * Slot 30.
     *
     * @param pObject The object whose alternation finished, which is not read.
     * @ghidraAddress 0x00273f28
     */
    virtual void OnUnknownSlot30(Rnd::Object *pObject);

    /**
     * Bring up the next screen once this one has exited.
     *
     * Slot 36. After a back command the mode screen is brought up. Otherwise the selected index
     * is written to GameParams::mDifficulty and the solo stages screen is brought up.
     *
     * @ghidraAddress 0x00274058
     */
    virtual void OnUnknownSlot36();

    /**
     * Resolve the base views and add the three buttons, `smgs_01.but` through `smgs_03.but`,
     * labelled `ms_easy`, `ms_normal`, and `ms_expert`.
     *
     * Slot 38.
     *
     * @ghidraAddress 0x00273310
     */
    virtual void ResolveContainerViews();

private:
    MetButtonList *mButtonList; // +0x8c
};
