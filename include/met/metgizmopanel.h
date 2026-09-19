#pragma once

#include "met/metscreen.h"

/**
 * Base of the four gizmo panels that frame the front end.
 *
 * `13MetGizmoPanel` in the RTTI descriptor at `0x00901c30`, with MetScreen as its one public
 * non-virtual base at offset 0. The 39-entry vtable is at `0x007f4340`, which is the same length
 * as the MetScreen table, so the class declares no virtual of its own.
 *
 * Four classes derive from the class, MetEndGameGizmoScreen, MetLeftGizmoScreen,
 * MetLeftGizmoSmallScreen, and MetRightGizmoScreen.
 *
 * The constructor is at `0x00276d38`. It takes the renderer, the load priority, and the three
 * names, and all four children call it. An earlier pass recorded the constructor as inline and
 * folded into the derived constructors, which was wrong. The other four functions that write this
 * vtable are the four child destructors restoring the base vptr during teardown, which is the
 * ordinary destruction sequence rather than construction.
 *
 * The class owns a vector of alternate view names. Each child registers its own into that vector
 * after the constructor returns, and each child destructor tears it down.
 *
 * The size is not recovered. No child places a second base after the MetGizmoPanel subobject and
 * none declares a data member of its own, so nothing fixes the width, and it is at least the 0x8c
 * of MetScreen.
 *
 * Three slots differ from the MetScreen table, and none of the three has a recovered name.
 *
 *  - 1 `0x0027b218` the destructor.
 *  - 26 `0x0027b3b0` replaces an empty MetScreen slot.
 *  - 27 `0x0027b388` replaces an empty MetScreen slot.
 *  - 38 `0x00276d90` replaces the view-resolving routine at `0x0038b1b0`.
 */
class MetGizmoPanel : public MetScreen {
public:
    /**
     * @ghidraAddress 0x0027b218
     */
    virtual ~MetGizmoPanel();
};
