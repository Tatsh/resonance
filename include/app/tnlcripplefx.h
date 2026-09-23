#pragma once

#include <vector>

class TnlPlayer;
namespace Rnd {
class ParticleSys;
class TransAnim;
class View;
} // namespace Rnd

/**
 * Crippler effect that flies along a path and hits a set of players.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the objects it drives, "cripfx<n>.view", "cripfx<n>.path", and
 * "cripfx<n>.ps", and from the "SND_CRIPPLER_HIT" sound the frame routine plays.
 *
 * The frame routine at `0x0043e500` is not reconstructed. It drives the view at mRate times the
 * song frame and, while running, drives the path. Once the path frame runs more than 6500 frames
 * ahead of the song it sets the path rate to `-3 * mRate`. Once the path frame falls behind the
 * song, it plays "SND_CRIPPLER_HIT" once per target, stores the song frame as the target's crippler
 * frame through `0x00457008`, and hands the target's Player to `0x00170c40` on the object at
 * `+0x34` of the game world. The next frame hides the view and releases the particles. The
 * GrooveWorld member at `+0x34` has no type in the tree, and `0x00170c40` belongs to that
 * unrecovered class.
 *
 * AppTunnel allocates three, 0x28 bytes each, and stores them in its vector at `+0x30`. The
 * destructor at `0x00456810` is the implicit one and is not written.
 */
class TnlCrippleFX {
public:
    /**
     * Resolve the view, path, and system of one crippler and hide the view.
     *
     * @param nIndex The crippler.
     * @param flRate The view rate. AppTunnel passes its `+0x144`.
     * @ghidraAddress 0x0043e1f0
     */
    TnlCrippleFX(int nIndex, float flRate);

    /**
     * Launch the crippler at a set of players.
     *
     * The path runs at `6 * mRate` and is offset to arrive at frame flFrame at song frame flFrame.
     * AppTunnel's message handler inlines this at `0x00448898`, and the out-of-line copy has no
     * caller.
     *
     * @param targets The players to hit.
     * @param flFrame The launch frame.
     * @ghidraAddress 0x004568b8
     */
    void Start(const std::vector<TnlPlayer *> &targets, float flFrame);

private:
    // AppTunnel::StartCrippleFX() reads mState to find an idle crippler.
    friend class AppTunnel;

    // Phase of the frame routine. Running follows Start(), returning follows the path reversal,
    // hit follows the path falling behind the song, and the frame after the hit returns to idle.
    enum State { kStateIdle = 0, kStateRunning = 1, kStateReturning = 2, kStateHit = 3 };

    Rnd::View *mView;               // "cripfx<n>.view".
    Rnd::TransAnim *mPath;          // "cripfx<n>.path".
    int mUnknown08;                 // +0x08, never written or read by the recovered routines.
    Rnd::ParticleSys *mParticleSys; // "cripfx<n>.ps".
    std::vector<TnlPlayer *> mTargets;
    State mState;
    float mHitFrame;
    float mRate;
};
