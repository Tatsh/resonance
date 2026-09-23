#pragma once

#include "met/metgizmopanel.h"

/**
 * Gizmo panel of the end-game screens.
 *
 * `21MetEndGameGizmoScreen` in the RTTI descriptor at `0x00901ef0`, with MetGizmoPanel as its one
 * public non-virtual base at offset 0. The class declares no data member. The 39-entry vtable at
 * `0x007f3e40` is the same length as the MetGizmoPanel table, and the class declares no new
 * virtual.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetGizmoPanel
 * table are 9 and 33.
 */
class MetEndGameGizmoScreen : public MetGizmoPanel {
public:
    /**
     * Construct the screen.
     *
     * Supplies `egg` for the screen name, `metagame/shared` for the directory, and
     * `end_game_gizmo` for the container, and registers the alternate view `egg_eq.view`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x002779f0
     */
    MetEndGameGizmoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Release the screen. The body is empty, and MetGizmoPanel's destructor is expanded in it.
     *
     * @ghidraAddress 0x0027bd58
     */
    virtual ~MetEndGameGizmoScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0027bec8
     */
    static MetEndGameGizmoScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Hide `egg_eq.view` and begin the exit.
     *
     * Slot 9. The view is resolved by name again rather than read from mViews.
     *
     * @ghidraAddress 0x0027bfd0
     */
    virtual void BeginExit();

    /**
     * Show `egg_eq.view`.
     *
     * Slot 33. The view is resolved by name again rather than read from mViews.
     *
     * @ghidraAddress 0x0027bf50
     */
    virtual void OnUnknownSlot33();
};
