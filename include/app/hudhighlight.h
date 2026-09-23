#pragma once

namespace Rnd {
class Mat;
class Mesh;
} // namespace Rnd

/**
 * Highlight box of the head-up display, a quad whose corners glide to a target rectangle.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `hilite_box` objects it resolves. The head-up display panel
 * embeds one at `+0x90`.
 *
 * The box mesh has sixteen vertices in four corner groups of four. A move gives every group a
 * constant horizontal speed that moves its first vertex to the target corner in the requested time,
 * and moves the group vertically along the straight line from that vertex to the target. The box
 * fades in over the same time.
 */
class HudHighlight {
public:
    /**
     * Resolve the box and its material and hide the box.
     *
     * @ghidraAddress 0x00417170
     */
    HudHighlight();

    /**
     * Start moving the box's corners to a rectangle over a time.
     *
     * The move starts at the next SetFrame(), and the alpha starts from 0. The title is inferred.
     *
     * @param flLeft The target left edge.
     * @param flTop The target top edge.
     * @param flRight The target right edge.
     * @param flBottom The target bottom edge.
     * @param flDuration The time the move takes, in the units SetFrame() receives.
     * @ghidraAddress 0x00417388
     */
    void MoveTo(float flLeft, float flTop, float flRight, float flBottom, float flDuration);

    /**
     * Put the box's corners on a rectangle at once.
     *
     * The title is inferred.
     *
     * @param flLeft The left edge.
     * @param flTop The top edge.
     * @param flRight The right edge.
     * @param flBottom The bottom edge.
     * @ghidraAddress 0x00417570
     */
    void JumpTo(float flLeft, float flTop, float flRight, float flBottom);

    /**
     * Advance a move in progress and its fade.
     *
     * The first call after MoveTo() records the start and the end time and does nothing else. Each
     * later call moves the corners by the time elapsed since the previous call, stopping at the end
     * time, and raises the alpha by the same share of the move, up to 1.
     *
     * @param flTime The current time.
     * @ghidraAddress 0x00417690
     */
    void SetFrame(float flTime);

    /**
     * Show or hide the box.
     *
     * The constructor is the one caller. The title is inferred.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00429d30
     */
    void SetShowing(int nShowing);

private:
    // Corner groups of the box, and vertices in each group.
    static constexpr int kCornerCount = 4;
    static constexpr int kCornerVertexCount = 4;
    // Edges of a rectangle, left, top, right, and bottom.
    static constexpr int kEdgeCount = 4;

    Rnd::Mesh *mMesh; // `HUD1 hilite_box.mesh`
    Rnd::Mat *mMat;   // `HUD hilite_box.mat`
    // Set by MoveTo() until the next SetFrame() records the start.
    int mStarting;
    // Set once a move has run to its end time.
    int mDone;
    float mEndTime;
    float mDuration;
    // The time SetFrame() last moved the corners at.
    float mLastTime;
    // The mesh vertex indices of each corner group. The first vertex of a group is the one a move
    // steers.
    unsigned char mCornerVerts[kCornerCount][kCornerVertexCount];
    // The line each corner follows, y = mSlope * x + mIntercept.
    float mSlope[kCornerCount];
    float mIntercept[kCornerCount];
    // Horizontal speed of each corner.
    float mVelocity[kCornerCount];
    // Alpha gained per unit of time.
    float mFadeRate;
    float mAlpha;
    // The rectangle MoveTo() last received.
    float mTarget[kEdgeCount];
};
