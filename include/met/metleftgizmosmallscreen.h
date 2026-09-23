#pragma once

#include "met/metgizmopanel.h"

/**
 * Small gizmo panel on the left of the front end.
 *
 * `23MetLeftGizmoSmallScreen` in the RTTI descriptor at `0x00901be0`, with MetGizmoPanel as its one
 * public non-virtual base at offset 0. The class declares no data member. The 39-entry vtable at
 * `0x007f40c0` is the same length as the MetGizmoPanel table, and the class declares no new
 * virtual.
 *
 * Apart from the type function and the destructor, the class overrides no slot.
 */
class MetLeftGizmoSmallScreen : public MetGizmoPanel {
public:
    /**
     * Construct the screen.
     *
     * Supplies `lls` for the screen name, `metagame/shared` for the directory, and
     * `left_panel_small` for the container, and registers the alternate views `lls_gyro.view` and
     * `lls_gizmo_kscope.view`.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00277280
     */
    MetLeftGizmoSmallScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * Release the screen. The body is empty, and MetGizmoPanel's destructor is expanded in it.
     *
     * @ghidraAddress 0x0027b738
     */
    virtual ~MetLeftGizmoSmallScreen();

    /**
     * Build the screen on the heap.
     *
     * @param pRenderer The front-end renderer the screen registers on.
     * @param nPriority The load priority.
     * @return The new screen.
     * @ghidraAddress 0x0027b8a8
     */
    static MetLeftGizmoSmallScreen *New(MetRenderer *pRenderer, int nPriority);
};
