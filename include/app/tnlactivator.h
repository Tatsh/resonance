#pragma once

#include "app/linearramp.h"
#include "app/tnlcatcher.h"
#include "app/tnlpointer.h"
#include "os/hxstr.h"

class TnlPlayer;
namespace Rnd {
class Mesh;
class ParticleSys;
class View;
} // namespace Rnd

/**
 * Activator of one player, the object that rides the player's tunnel seeker from track to track.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from its objects, "activator%d", "activator rot%d", and "activator
 * fx%d".
 *
 * MoveToTrack() turns the rotation view toward the new track at 0.15 radians per update and hides
 * the player's ghost on the old track. Once the turn and the seeker offset ramp have both settled,
 * Update() shows the ghost on the new track. Before the song starts the "act_<c> intro.view"
 * animation plays, and after frame -1920 the activator mesh appears and the rotation view is
 * removed from the owner's local view.
 *
 * TnlPlayer embeds one, 0xe0 bytes, at `+0x34`.
 */
class TnlActivator {
public:
    /**
     * Resolve every object, attach the pointer and the catcher, and hide the mesh.
     *
     * The seeker transform offset is -500, or -550 with at least two local players. The rotation
     * view joins the owner's local view when the owner has a camera.
     *
     * @param nIndex The player index and the seeker index.
     * @param colorName The player's colour name, taken by value.
     * @param pOwner The player object that embeds this one.
     * @ghidraAddress 0x0043b3d8
     */
    TnlActivator(int nIndex, HxStr colorName, TnlPlayer *pOwner);

    /**
     * Advance the turn, the seeker offsets, the blink, the intro, the catcher, and the pointer.
     *
     * @param flFrame The song position.
     * @param flScaledFrame The song position scaled by the AppTunnel factor at `+0x144`. The
     *                      catcher and the pointer run on it.
     * @ghidraAddress 0x0043baf8
     */
    void Update(float flFrame, float flScaledFrame);

    /**
     * Show or hide the rotation view.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00455ac8
     */
    void SetRotShowing(int nShowing);

    /**
     * Attach "leader.ps" to the effect view and show it, or detach and hide it.
     *
     * @param nLeader Non-zero to attach and show.
     * @ghidraAddress 0x00455a40
     */
    void SetLeader(int nLeader);

    /**
     * Record whether the ghost is wanted, and show or hide it unless mLevel is set.
     *
     * The ghost is mGhostView placed on track mTrack through AppTunnel.
     *
     * @param nGhost Non-zero to show.
     * @ghidraAddress 0x00455c98
     */
    void SetGhost(int nGhost);

    /**
     * Hide or restore the rotation view and the ghost together.
     *
     * mGhost is restored after the inner SetGhost() call. The program lists no caller.
     *
     * @param nSuppressed Non-zero to hide both.
     * @ghidraAddress 0x00455af8
     */
    void SetSuppressed(int nSuppressed);

    /**
     * Start a turn toward a track and set the level and the instrument kind.
     *
     * Track n sits at `(1 - n / 8)` of a full turn. The seeker offset ramp moves toward a third of
     * nLevel, the pointer shows the pair of nKind, and the catcher shows for kinds 2 and 5 only.
     * The program lists no caller.
     *
     * @param nLevel The level. A non-zero level hides the ghost.
     * @param nKind The instrument kind.
     * @param flTrack The track.
     * @ghidraAddress 0x00455b70
     */
    void MoveToTrack(int nLevel, int nKind, float flTrack);

private:
    int mIndex;
    Rnd::View *mRotView;   // "activator rot%d".
    Rnd::Mesh *mMesh;      // "activator%d", the seeker mesh.
    Rnd::View *mIntroView; // "act_<c> intro.view".
    Rnd::View *mFxView;    // "activator fx%d".
    TnlPlayer *mOwner;
    TnlPointer mPointer;
    TnlCatcher mCatcher;
    int mIntroState;
    int mTurning;
    float mTargetAngle;
    float mAngle;
    int mTrack;
    int mLevel;
    LinearRamp mOffsetRamp; // Seeker offsets, 0 through -225 over 480.
    int mBlink;             // Non-zero dims the materials for frames 121 through 239 of every 240.
    float mTransOffset;
    int mGhost;
    int mSuppressed;
    Rnd::View *mGhostView; // "ghost<n>.view".
    Rnd::ParticleSys *mLeader;
};
