#pragma once

#include "math/vector3.h"
#include "rnd/mesh.h"
#include "rnd/view.h"

namespace Rnd {
class Mat;
} // namespace Rnd

/**
 * Flash that lifts one tunnel panel out of the wall and fades it away.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from its mesh name prefix "pnl", from the "tunnel erase%d" material it
 * draws with, and from the delayed-panel record at `0x00457648` that fires it through AppTunnel.
 *
 * Start() copies the geometry of one ring section onto a private mesh and places it in
 * "tnl transparent". Update() then runs three timed phases. The panel slides from its offset
 * position onto the tunnel wall while it fades in, stays with an additive blend, and fades out
 * before the mesh drops back to its empty faces.
 *
 * AppTunnel allocates one per entry of its panel vector at `+0x3c`, 0x30 bytes each.
 */
class TnlPanelFX {
public:
    /**
     * Build the panel mesh and resolve the material for one panel slot.
     *
     * The mesh name is "pnl" followed by NextAppTunnelName(). The mesh reads Z without writing it,
     * does not draw until Start(), and is added to "tnl transparent". The material is
     * "tunnel erase<nIndex>". When no such material is loaded, a new one of that name is created
     * as a copy of "tunnel erase0".
     *
     * @param nIndex The panel slot, also the material number.
     * @ghidraAddress 0x0043c688
     */
    explicit TnlPanelFX(int nIndex);

    /**
     * Take the mesh out of "tnl transparent" and delete it.
     *
     * AppTunnel's destructor inlines the body. The deleting copy at `0x00456418` has no caller.
     *
     * @ghidraAddress 0x00456418
     */
    ~TnlPanelFX() {
        mView->RemoveDraw(mMesh);
        delete mMesh;
    }

    /**
     * Show the panel over one ring section and begin the rise phase.
     *
     * The mesh shares the faces of the ring section and copies its vertices. The path frame is the
     * middle of the bar, `nSlice * 1920 + 960`. mOffset becomes the ring point halfway to the next
     * ring less the path origin at the same frame.
     *
     * @param nRing The ring of the section.
     * @param nSlice The slice of the section, also the bar.
     * @param nForward Non-zero to rise along mOffset, zero to rise against it.
     * @ghidraAddress 0x0043cb78
     */
    void Start(int nRing, int nSlice, int nForward);

    /**
     * Advance the phase the panel is in.
     *
     * While idle the routine only records flFrame. The rise lasts 200 frames and moves the mesh
     * origin from `mOffset * mDirection` to zero while the alpha climbs twice as fast as the
     * phase. The hold lasts 400 frames with an additive blend and the fade 200 frames, and both
     * take the alpha from 1 to 0. The end of the fade hides the mesh and makes it the owner of its
     * faces again.
     *
     * @param flFrame The current frame.
     * @ghidraAddress 0x0043cd38
     */
    void Update(float flFrame);

    /**
     * Report whether the panel waits for Start().
     *
     * AppTunnel's routine at `0x00457648` tests the state inline to find a free panel, and no
     * out-of-line copy exists.
     *
     * @return True while idle.
     */
    bool IsIdle() const {
        return mState == kStateIdle;
    }

private:
    // Phase Update() runs. Idle is hidden and waiting for Start(), rise slides into the wall and
    // fades in, hold shows with an additive blend, and fade fades out.
    enum State { kStateIdle = 0, kStateRise = 1, kStateHold = 2, kStateFade = 3 };

    float mStateFrame; // Frame the current phase began. Starts at 1e9.
    State mState;
    int mIndex;
    float mDirection; // 1 or -1, from Start().
    Vector3 mOffset;  // Start offset of the rise, before mDirection applies.
    Rnd::Mesh *mMesh;
    Rnd::Mat *mMat;
    Rnd::View *mView; // "tnl transparent".
};
