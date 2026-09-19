#pragma once

#include "met/metgizmopanel.h"

/**
 * Gizmo panel that frames the front end.
 *
 * `19MetRightGizmoScreen` in the RTTI descriptor at `0x008f2a00`, with MetGizmoPanel as its one
 * public non-virtual base at offset 0. The class declares no data member of its own, and the
 * 39-entry vtable at `0x007f3f80` is the same length as the MetGizmoPanel table, so it
 * declares no virtual of its own either.
 *
 * The constructor at `0x00277628` takes only the renderer and the load priority. It runs the
 * MetGizmoPanel constructor at `0x00276d38` with `rpl` for the screen name,
 * `metagame/shared` for the directory, and `right_panel_large` for the container, and then
 * registers the alternate views it owns, `rpl_gizmo.view`, `rpl_gizmo_eq.view`, and
 * `rpl_gizmo_kscope.view`. Those names go into a vector that MetGizmoPanel owns, which is what the
 * destructor tears down.
 *
 * The destructor at `0x0027b9b8` releases that vector, restores the MetGizmoPanel vptr, and
 * runs the MetGizmoPanel destructor.
 *
 * Apart from the type function and the destructor, the slots that differ from the
 * MetGizmoPanel table are 9 `0x0027bc38` and 33 `0x0027bbb0`.
 */
class MetRightGizmoScreen : public MetGizmoPanel {
public:
    /**
     * Construct the screen.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @ghidraAddress 0x00277628
     */
    MetRightGizmoScreen(MetRenderer *pRenderer, int nPriority);

    /**
     * @ghidraAddress 0x0027b9b8
     */
    virtual ~MetRightGizmoScreen();
};
