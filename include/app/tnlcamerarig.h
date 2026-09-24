#pragma once

#include <vector>

#include "app/linearramp.h"
#include "math/vector3.h"
#include "rnd/cam.h"

namespace Rnd {
class View;
} // namespace Rnd

/**
 * Main tunnel cameras and the blend between the intro pose and the playing pose.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the cameras it drives, "tnl cam1", "tnl cam1z", and "outer cam1"
 * onward, and from the intro path "tnl cam intro<n>.tnm" it samples.
 *
 * A ramp from 0 to 1 blends the pitch, the position, the screen rectangle, and the field of view
 * of "tnl cam1" from the intro pose to the playing pose. The two main cameras and the first outer
 * camera share the field of view and the screen rectangle. ZoomIn() runs the ramp towards the
 * playing pose and hides the views of players 2 to 4. ZoomOut() runs it back, and the views show
 * again once the ramp settles.
 *
 * AppTunnel allocates one, 0xa0 bytes, and stores it at `+0xc4`. The destructor at `0x00457010`
 * is the implicit one and is not written.
 */
class TnlCameraRig {
public:
    /**
     * Resolve the cameras and views and record the two poses.
     *
     * The intro pose comes from frame 0 of "tnl cam intro<nPlayerCount>.tnm" and from the current
     * screen rectangle and field of view of "tnl cam1". The playing pose is a pitch of 28 degrees,
     * the position `(0, -0.5, 0)`, the rectangle `(0, 0.1, 1, 0.8)`, and a field of view of 80
     * degrees. With nSkipIntro set the ramp jumps to the playing pose and ZoomIn() hides the other
     * players' cameras and views.
     *
     * @param nPlayerCount The number of players, each with an outer camera and a local view.
     * @param nSkipIntro Non-zero to start in the playing pose. AppTunnel passes its `+0xc8`.
     * @ghidraAddress 0x00440d30
     */
    TnlCameraRig(int nPlayerCount, int nSkipIntro);

    /**
     * Run the ramp towards the playing pose and hide the outer cameras and views of players 2 to 4.
     *
     * @ghidraAddress 0x004414a0
     */
    void ZoomIn();

    /**
     * Run the ramp back towards the intro pose.
     *
     * The views of players 2 to 4 show again once the ramp settles. AppTunnel inlines this, and
     * the out-of-line copy has no caller.
     *
     * @ghidraAddress 0x00457118
     */
    void ZoomOut();

    /**
     * Advance the ramp and apply the blended pose.
     *
     * While the ramp moves, "tnl cam1" takes the blended pitch and position, and "tnl cam1", "tnl
     * cam1z", and the first outer camera take the blended field of view and screen rectangle. Once
     * the ramp settles after ZoomOut(), the outer cameras and views of players 2 to 4 show.
     *
     * @param flTime The current time.
     * @ghidraAddress 0x00441558
     */
    void SetFrame(float flTime);

private:
    // Phase after the ramp settles. Zoomed in follows ZoomIn(), zooming out follows ZoomOut(),
    // and settled follows the next SetFrame() in which the ramp does not move.
    enum State { kStateZoomedIn = 0, kStateZoomingOut = 1, kStateSettled = 2 };

    // Index into mMainCams.
    enum MainCam { kMainCamNormal = 0, kMainCamZoom = 1, kMainCamCount = 2 };

    Rnd::Cam *mMainCams[kMainCamCount];   // "tnl cam1" and "tnl cam1z".
    std::vector<Rnd::Cam *> mOuterCams;   // "outer cam<n>", four slots.
    std::vector<Rnd::View *> mLocalViews; // "tnl local<n>.view", four slots.
    int mPlayerCount;
    State mState;
    LinearRamp mRamp; // 0 at the intro pose and 1 at the playing pose.
    float mIntroPitch;
    float mPlayPitch;
    Vector3 mIntroPos;
    Vector3 mPlayPos;
    Rnd::Cam::Rect mIntroRect;
    Rnd::Cam::Rect mPlayRect;
    float mIntroFov;
    float mPlayFov;
};
