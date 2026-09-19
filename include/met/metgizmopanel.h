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
 * The size is not recovered. No child places a second base after the MetGizmoPanel subobject, so
 * nothing in the RTTI fixes the width, and the constructor is inline and appears folded into each
 * of the five derived constructors at `0x00276d38`, `0x0027b4b8`, `0x0027b738`, `0x0027b9b8`, and
 * `0x0027bd58` rather than as one routine. The size is therefore at least the 0x8c of MetScreen.
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
