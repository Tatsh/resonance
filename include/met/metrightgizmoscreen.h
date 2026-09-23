#pragma once

#include "met/metgizmopanel.h"

/**
 * Large gizmo panel on the right of the front end.
 *
 * `19MetRightGizmoScreen` in the RTTI descriptor at `0x008f2a00`, with MetGizmoPanel as its one
 * public non-virtual base at offset 0. The class declares no data member. The 39-entry vtable at
 * `0x007f3f80` is the same length as the MetGizmoPanel table, and the class declares no new
 * virtual.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetGizmoPanel
 * table are 9 and 33.
 */
class MetRightGizmoScreen : public MetGizmoPanel {
public:
    /**
     * Construct the screen.
     *
     * Supplies `rpl` for the screen name, `metagame/shared` for the directory, and
     * `right_panel_large` for the container, and registers the alternate views `rpl_gizmo.view`,
     * `rpl_gizmo_eq.view`, and `rpl_gizmo_kscope.view`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00277628
     */
    MetRightGizmoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Release the screen. The body is empty, and MetGizmoPanel's destructor is expanded in it.
     *
     * @ghidraAddress 0x0027b9b8
     */
    virtual ~MetRightGizmoScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0027bb28
     */
    static MetRightGizmoScreen *New(MetRenderer *pRenderer, int nPriority);

    /**
     * Hide `rpl_gizmo_eq.view` and begin the exit.
     *
     * Slot 9. The view is resolved by name again rather than read from mViews.
     *
     * @ghidraAddress 0x0027bc38
     */
    virtual void BeginExit();

    /**
     * Show `rpl_gizmo_eq.view`.
     *
     * Slot 33. The view is resolved by name again rather than read from mViews.
     *
     * @ghidraAddress 0x0027bbb0
     */
    virtual void OnUnknownSlot33();
};
