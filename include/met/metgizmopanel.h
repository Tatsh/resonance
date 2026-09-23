#pragma once

#include <vector>

#include "met/metscreen.h"
#include "os/hxstr.h"

namespace Rnd {
class View;
}

/**
 * Base of the four gizmo panels that frame the front end.
 *
 * `13MetGizmoPanel` in the RTTI descriptor at `0x00901c30`, with MetScreen as its one public
 * non-virtual base at offset 0. The 39-entry vtable at `0x007f4340` is the same length as the
 * MetScreen table, and the class declares no new virtual.
 *
 * Four classes derive from the class, MetEndGameGizmoScreen, MetLeftGizmoScreen,
 * MetLeftGizmoSmallScreen, and MetRightGizmoScreen. Each child fills mViewNames after the
 * constructor returns. No child declares a data member, and each child's New() allocates 0xa4
 * bytes.
 *
 * The four child destructors expand this class's destructor and restore this class's vptr during
 * teardown.
 *
 * The translation unit spans `0x00276d38` to `0x0027c2e0` and includes all five classes, with
 * their type functions, per-unit copies of MsgSink routines, and template library emissions.
 *
 * Apart from the type function and the destructor, the slots that differ from the MetScreen table
 * are 26, 27, and 38.
 */
class MetGizmoPanel : public MetScreen {
public:
    /**
     * Construct the panel with no alternate views.
     *
     * @param pRenderer The front-end renderer this screen registers on.
     * @param nPriority The load priority.
     * @param name The screen name.
     * @param directory The directory the container loads from.
     * @param container The container name, without its `.rnd` suffix.
     * @ghidraAddress 0x00276d38
     */
    MetGizmoPanel(MetRenderer *pRenderer,
                  int nPriority,
                  const HxStr &name,
                  const HxStr &directory,
                  const HxStr &container);

    /**
     * Release the two view vectors.
     *
     * @ghidraAddress 0x0027b218
     */
    virtual ~MetGizmoPanel();

    /**
     * Set every alternate view to one animation frame.
     *
     * Slot 26.
     *
     * @param flTime The frame.
     * @ghidraAddress 0x0027b3b0
     */
    virtual void OnUnknownSlot26(float flTime);

    /**
     * Forward to OnUnknownSlot26().
     *
     * Slot 27.
     *
     * @param flTime The frame.
     * @ghidraAddress 0x0027b388
     */
    virtual void UpdateIdleAnimation(float flTime);

    /**
     * Resolve the base views, and then each name in mViewNames into mViews as a Rnd::View.
     *
     * Slot 38. mViews is resized to the length of mViewNames first.
     *
     * @ghidraAddress 0x00276d90
     */
    virtual void ResolveContainerViews();

protected:
    std::vector<HxStr> mViewNames;   // +0x8c, filled by the child constructors
    std::vector<Rnd::View *> mViews; // +0x98, resolved from mViewNames by slot 38
};
