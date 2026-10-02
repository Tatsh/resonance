#pragma once

#include "met/metgizmopanel.h"

/**
 * Large gizmo panel on the left of the front end.
 *
 * `18MetLeftGizmoScreen` in the RTTI descriptor at `0x008eedb8`, with MetGizmoPanel as its one
 * public non-virtual base at offset 0. The class declares no data member. The 39-entry vtable at
 * `0x007f4200` is the same length as the MetGizmoPanel table, and the class declares no new
 * virtual.
 *
 * Apart from the type function and the destructor, the class overrides no slot.
 */
class MetLeftGizmoScreen : public MetGizmoPanel {
public:
    /**
     * Construct the screen.
     *
     * Supplies `lll` for the screen name, `metagame/shared` for the directory, and
     * `left_panel_large` for the container, and registers the alternate views `lll_gizmo.view` and
     * `lll_gizmo_kscope.view`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00276ed8
     */
    MetLeftGizmoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Release the screen. The body is empty, and MetGizmoPanel's destructor is expanded in it.
     *
     * @ghidraAddress 0x0027b4b8
     */
    virtual ~MetLeftGizmoScreen();

    /**
     * Build the screen on the heap.
     *
     * MetScreen::CreateMainMenuScreens() is the caller.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0027b628
     */
    static MetLeftGizmoScreen *New(MetRenderer *pRenderer, int nPriority);
};
