#pragma once

#include <vector>

#include "math/color.h"

namespace Rnd {
class Mesh;
class String;
class View;
} // namespace Rnd

/**
 * Glowing ribbon with a head mesh that snakes along one lane of the tunnel.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the objects it uses, "snake.mat" and "snake head.mesh".
 *
 * The ribbon is a private Rnd::String of 16 points in "tnl transparent", and the head is a private
 * copy of "snake head.mesh" drawn by the ribbon. Point i trails the head by `100 * i` song frames,
 * and each point sits on the lane at its frame, swinging on a sine of the frame. Random pulses of
 * glow run down the ribbon and fade by 0.15 per update.
 *
 * AppTunnel allocates three, 0x50 bytes each, and stores them in its vector at `+0x54`.
 */
class TnlSnake {
public:
    /**
     * Build the ribbon and the head.
     *
     * The ribbon and the head are named with NextAppTunnelName(). The ribbon draws with
     * "snake.mat" at width 0.2 and starts hidden.
     *
     * @ghidraAddress 0x0043e6a0
     */
    TnlSnake();

    /**
     * Take the ribbon out of "tnl transparent" and delete the ribbon and the head.
     *
     * @ghidraAddress 0x0043ed50
     */
    ~TnlSnake();

    /**
     * Show the ribbon on a lane for 11520 song frames from flFrame.
     *
     * AppTunnel inlines this, and the out-of-line copy has no caller.
     *
     * @param flFrame The start frame.
     * @param nRing The lane.
     * @param color The ribbon colour.
     * @param flPhase The phase of the swing, in radians.
     * @param flAmplitude The amplitude of the swing.
     * @ghidraAddress 0x00456960
     */
    void Start(float flFrame, int nRing, const Color &color, float flPhase, float flAmplitude);

    /**
     * Move the ribbon to the song frame.
     *
     * Returns at once while idle or when flFrame repeats the previous call. The head runs at four
     * times the song speed from the start frame, and the ribbon hides once the head passes the end
     * frame. Each point is placed at its frame, coloured with its glow added to mColor, and its
     * glow decays. The head mesh moves to the first point. With probability 0.2 a new pulse lands
     * on four neighbouring points.
     *
     * @param flFrame The song frame.
     * @ghidraAddress 0x0043eec0
     */
    void Update(float flFrame);

private:
    // AppTunnel::StartSnake() reads mStartFrame to find an idle ribbon.
    friend class AppTunnel;

    // Place point nIndex at the lane position of frame nFrame. A frame another point already
    // shows is copied from that point.
    void SetPointFrame(int nFrame, int nIndex);

    float mStartFrame; // 1e9 while idle.
    float mEndFrame;
    float mLastFrame;
    Rnd::String *mString;
    Rnd::Mesh *mHead;
    Rnd::View *mView; // "tnl transparent".
    int mRing;
    int mUnknown1c; // +0x1c, never written or read by the recovered routines.
    Color mColor;
    float mPhase;
    float mAmplitude;
    std::vector<float> mGlow;      // Extra brightness of each point.
    std::vector<int> mPointFrames; // Frame each point was last placed at.
};
