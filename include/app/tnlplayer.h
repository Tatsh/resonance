#pragma once

#include "app/tnlactivator.h"
#include "app/tnlgridmarkers.h"
#include "app/tnlsabretrail.h"
#include "app/tnlseekerfade.h"

class AppTunnel;
class Player;
namespace Rnd {
class Cam;
class TransAnim;
class Transformable;
class View;
} // namespace Rnd

/**
 * Everything AppTunnel drives in the tunnel for one player.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the per-player objects it resolves, "tnl cam%d", "tnl cam fx%d",
 * and "tnl local%d.view". AppTunnel's constructor allocates one per player.
 *
 * The object is 0x1a0 bytes. It embeds the activator at `+0x34`, the grid markers at `+0x114`, the
 * sabre trail at `+0x128`, and the seeker fade at `+0x170`. The destructor at `0x004409d8` is the
 * implicit one and is not written.
 *
 * A player without a local slot has no camera objects. In jukebox mode the camera intro path is
 * dropped as well.
 */
class TnlPlayer {
public:
    /**
     * Resolve the player's paths, camera objects, and views, and build the embedded objects.
     *
     * The effect transforms start at the identity. The seeker of nIndex follows "tnl cam
     * slide<n>" for a player with a local slot and follows no transform otherwise.
     *
     * @param pPlayer The player.
     * @param nIndex The player's index in the world and its seeker index.
     * @param pTunnel The tunnel that allocated this object.
     * @ghidraAddress 0x00440020
     */
    TnlPlayer(Player *pPlayer, int nIndex, AppTunnel *pTunnel);

    /**
     * Advance the embedded objects and the camera paths.
     *
     * The camera intro path drives mCam while flFrame is not positive. The two crippler paths
     * run for 5000 frames from mCrippleFrame.
     *
     * @param flFrame The song position.
     * @param flScaledFrame The song position scaled by the AppTunnel factor at `+0x144`.
     * @ghidraAddress 0x00440b48
     */
    void Update(float flFrame, float flScaledFrame);

    /**
     * Start the two crippler paths from a frame.
     *
     * TnlCrippleFX::SetFrame() calls it for each player the crippler hits. The out-of-line copy is
     * emitted in AppTunnel's translation unit.
     *
     * @param flFrame The frame the paths start from.
     * @ghidraAddress 0x00457008
     */
    void SetCrippleFrame(float flFrame) {
        mCrippleFrame = flFrame;
    }

private:
    friend class AppTunnel;
    friend class TnlActivator;
    friend class TnlArrow;

    int mPlayerNum; // Player::Slot2() plus 1.
    float mCrippleFrame;
    Rnd::TransAnim *mCrippleActPath;  // "crip act path".
    Rnd::Transformable *mActivatorFx; // "activator fx%d".
    Rnd::TransAnim *mCrippleCamPath;  // "crip cam path".
    Rnd::Transformable *mCamFx;       // "tnl cam fx%d".
    Rnd::Cam *mCam;                   // "tnl cam%d".
    Rnd::TransAnim *mCamIntro;        // "tnl cam intro<local player count>.tnm".
    Rnd::View *mLocalView;            // "tnl local%d.view".
    AppTunnel *mTunnel;
    int mUnknown28; // +0x28, zeroed and never read by the recovered routines.
    int mIndex;
    Player *mPlayer;
    TnlActivator mActivator;
    TnlGridMarkers mGridMarkers;
    TnlSabreTrail mSabreTrail;
    TnlSeekerFade mSeekerFade;
    int mUnknown198; // +0x198, never accessed by the recovered routines.
    int mUnknown19c; // +0x19c, never accessed by the recovered routines.
};
