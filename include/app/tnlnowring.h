#pragma once

#include <vector>

namespace Rnd {
class Mesh;
class View;
} // namespace Rnd

/**
 * Ring of "now" meshes around the tunnel, with a gap under each player.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the objects it drives, "nowring.view", "nowring rot.view", and
 * the meshes "nowmesh0" onward.
 *
 * Every mesh shows except the one each player's slot selects. Until the first SetFrame() past
 * frame -1920 the meshes are not refreshed on a slot change. That SetFrame() refreshes them,
 * resets the basis of "nowring rot.view" to the identity, and clears the pending flag.
 *
 * AppTunnel allocates one, 0x24 bytes, and stores it at `+0x14`. The destructor at `0x00455ef0`
 * is the implicit one and is not written.
 */
class TnlNowRing {
public:
    /**
     * Resolve the views and meshes and show every mesh.
     *
     * @param nMeshCount The number of meshes, "nowmesh0" through "nowmesh<nMeshCount - 1>".
     * @param nPlayerCount The number of player slots, each starting at mesh 0.
     * @ghidraAddress 0x0043c118
     */
    TnlNowRing(int nMeshCount, int nPlayerCount);

    /**
     * Record the mesh a player's slot hides, and refresh the meshes unless the reset is pending.
     *
     * AppTunnel inlines this at `0x004475d4`.
     *
     * @param nPlayer The player slot.
     * @param nMesh The index of the mesh to hide.
     * @ghidraAddress 0x00456268
     */
    void SetPlayerMesh(int nPlayer, int nMesh);

    /**
     * Run the pending reset once flFrame passes -1920.
     *
     * AppTunnel inlines this at `0x00446f1c`, and the out-of-line copy has no caller.
     *
     * @param flFrame The current frame.
     * @ghidraAddress 0x00456148
     */
    void SetFrame(float flFrame);

    /**
     * Turn "nowring rot.view" about its Y axis by whole eighths of a turn while the reset is
     * pending.
     *
     * The basis rows of the view's local transform become a rotation of `-nStep * 45` degrees, and
     * the translation row is unchanged. The degrees convert through a pi slightly below the float
     * nearest pi. SetFrame() inlines the call with nStep 0, and AppTunnel::PrepareLocalView()
     * inlines it with the view index. The out-of-line copy has no caller.
     *
     * @param nStep The number of eighths of a turn.
     * @ghidraAddress 0x00456028
     */
    void SetRotation(int nStep);

    /**
     * Show every mesh, then hide the mesh of each player slot.
     *
     * @ghidraAddress 0x004562a0
     */
    void RefreshMeshes();

    /**
     * Show or hide "nowring.view".
     *
     * AppTunnel inlines this in its constructor and its PlaybackToggleMsg handler, and the
     * out-of-line copy has no caller.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00455ff8
     */
    void SetShowing(int nShowing);

private:
    std::vector<int> mPlayerMeshes; // Index into mMeshes of the mesh each player slot hides.
    std::vector<Rnd::Mesh *> mMeshes;
    Rnd::View *mView;    // "nowring.view".
    Rnd::View *mRotView; // "nowring rot.view".
    int mResetPending;
};
