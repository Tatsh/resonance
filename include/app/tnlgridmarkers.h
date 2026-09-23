#pragma once

#include <vector>

class AppTunnel;
namespace Rnd {
class Drawable;
class Mesh;
} // namespace Rnd

/**
 * Sixteen copies of the grid meshes that march down one player's tunnel track.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the view it fills, "grid<n>.view", and the two meshes it copies,
 * "grid4" and "grid8".
 *
 * The markers start 240 frames apart from frame 0. A marker on a beat is a copy of "grid4" and
 * every other marker a copy of "grid8". Update() moves a marker more than 480 frames behind the
 * song position forward by 3840 frames, the length of the whole row, and hides a marker whose bar
 * AppTunnel::IsTrackBarLocked() reports for mTrack.
 *
 * TnlPlayer embeds one, 0x14 bytes, at `+0x114`.
 */
class TnlGridMarkers {
public:
    /**
     * One marker, 8 bytes.
     *
     * The structure emits no RTTI. The name is inferred.
     */
    struct Marker {
        /**
         * Delete the mesh.
         *
         * A copied marker deletes the same mesh again. The constructor copies only markers
         * without a mesh.
         */
        ~Marker();

        /**
         * Create a copy of a mesh and add it to a drawable.
         *
         * The copy is a new mesh from g_pfnNewMesh() under a NextAppTunnelName() name. A null
         * pSource sets mMesh to null and does nothing else.
         *
         * @param pSource The mesh to copy, or null.
         * @param pParent The drawable the copy is added to.
         * @ghidraAddress 0x00438fe0
         */
        void Init(Rnd::Mesh *pSource, Rnd::Drawable *pParent);

        /**
         * Place the mesh on a track at a song position and record the position.
         *
         * Does nothing without a mesh.
         *
         * @param nTrack The tunnel track.
         * @param nFrame The song position, in frames.
         * @ghidraAddress 0x00454fb0
         */
        void Place(int nTrack, int nFrame);

        Rnd::Mesh *mMesh; /*!< The copy, or null. */
        float mFrame;     /*!< Song position the mesh stands at, in frames. */
    };

    /**
     * Build the markers into the player's grid view on track 0.
     *
     * Builds nothing when "grid<n>.view" does not resolve. The view's draw list is cleared first.
     *
     * @param pTunnel The tunnel, queried by Update().
     * @param nPlayerNum The player number, selecting the view.
     * @ghidraAddress 0x00439150
     */
    TnlGridMarkers(AppTunnel *pTunnel, int nPlayerNum);

    /**
     * Move every marker onto another track at its current song position.
     *
     * The program lists no caller.
     *
     * @param nTrack The tunnel track.
     * @ghidraAddress 0x004550c8
     */
    void SetTrack(int nTrack);

    /**
     * Recycle the markers behind the song position and fade and show each one.
     *
     * A marker's alpha is `(3840 - mFrame + flFrame) / 1920`, capped at 0.8.
     *
     * @param flFrame The song position.
     * @ghidraAddress 0x00439718
     */
    void Update(float flFrame);

private:
    std::vector<Marker> mMarkers;
    int mTrack;
    AppTunnel *mTunnel;
};
