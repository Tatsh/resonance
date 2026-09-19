#pragma once

#include "met/metgizmopanel.h"

/**
 * Gizmo panel that frames the front end.
 *
 * `18MetLeftGizmoScreen` in the RTTI descriptor at `0x008eedb8`, with MetGizmoPanel as its one
 * public non-virtual base at offset 0. The class declares no data member of its own, and the
 * 39-entry vtable at `0x007f4200` is the same length as the MetGizmoPanel table, so it
 * declares no virtual of its own either.
 *
 * The constructor at `0x00276ed8` takes only the renderer and the load priority. It runs the
 * MetGizmoPanel constructor at `0x00276d38` with `lll` for the screen name,
 * `metagame/shared` for the directory, and `left_panel_large` for the container, and then
 * registers the alternate views it owns, `lll_gizmo.view` and `lll_gizmo_kscope.view`. Those names
 * go into a vector that MetGizmoPanel owns, which is what the destructor tears down.
 *
 * The destructor at `0x0027b4b8` releases that vector, restores the MetGizmoPanel vptr, and
 * runs the MetGizmoPanel destructor.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetGizmoPanel table are none.
 */
class MetLeftGizmoScreen : public MetGizmoPanel {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00276ed8
     */
    MetLeftGizmoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0027b4b8
     */
    virtual ~MetLeftGizmoScreen();
};
