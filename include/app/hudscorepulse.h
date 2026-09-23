#pragma once

class HudBadge;

namespace Rnd {
class Mesh;
} // namespace Rnd

/**
 * Pulse drawn over the leader's score on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. It occupies `+0x114` of HudPanel, whose
 * constructor runs the constructor. No descriptor, tag, or file path identifies the class, and its
 * name is inferred from the one object it resolves.
 *
 * MoveTo() is not written. It needs the player's colour name at Player `+0x24`, which game/player.h
 * declares as an int, and the material and local transform members of Rnd::Mesh.
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
     * Tints the mesh's material with HudColorFromName() of the player's colour name, copies the
     * world position of the badge's score mesh into the pulse mesh's local position, marks the
     * transform dirty, and shows the mesh. Overlay::OnLeaderChanged() is the caller. The title is
     * inferred.
     *
     * @param pBadge The leader's badge.
     * @ghidraAddress 0x0041c2b0
     */
    void MoveTo(HudBadge *pBadge);

private:
    Rnd::Mesh *mMesh; // `<layout> score pulse.mesh`
};
