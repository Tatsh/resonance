#pragma once

#include "rnd/mesh.h"

class HudBadge;

/**
 * Pulse drawn over the leader's score on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. It occupies `+0x114` of HudPanel, whose
 * constructor runs the constructor. No descriptor, tag, or file path identifies the class, and its
 * name is inferred from the one object it resolves.
 */
class HudScorePulse {
public:
    /**
     * Resolve `<layout> score pulse.mesh` and hide it.
     *
     * @ghidraAddress 0x0041bdb8
     */
    HudScorePulse();

    /**
     * Show the pulse over one badge's score, in its player's colour.
     *
     * Sets the emissive colour of the mesh's material to HudColorFromName() of the player's colour
     * name, copies the translation row of the score mesh's local transform into the pulse mesh's,
     * marks the transform dirty, and shows the mesh. Overlay::OnLeaderChanged() is the caller. The
     * title is inferred.
     *
     * @param pBadge The leader's badge.
     * @ghidraAddress 0x0041c2b0
     */
    void MoveTo(HudBadge *pBadge);

    /**
     * Hide the pulse.
     *
     * Overlay::OnLeaderChanged() inlines the body when no player leads, and no out-of-line copy
     * exists.
     */
    void Hide() {
        mMesh->SetShowing(0);
    }

private:
    Rnd::Mesh *mMesh; // `<layout> score pulse.mesh`
};
