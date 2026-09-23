#pragma once

#include <vector>

#include "math/vector3.h"
#include "os/hxstr.h"

namespace Rnd {
class ParticleSys;
class String;
} // namespace Rnd

/**
 * Line drawn through the gems of a run of bars on one tunnel track, with a glow at each gem.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the Rnd::String it draws, "sabre_<c>.str", where `<c>` is the first
 * letter of the player's colour name.
 *
 * Build() places one point per gem and fills each gap between gems with a point every 60 frames.
 * Update() reveals the points at 0.04 per frame from the first update, and a Pulse() makes the
 * revealed filler points past the pulse frame oscillate across the track and swells the gem glows.
 *
 * TnlPlayer embeds one, 0x48 bytes, at `+0x128`.
 */
class TnlSabreTrail {
public:
    /**
     * One point of the line, 0x20 bytes.
     *
     * The structure emits no RTTI. The name is inferred.
     */
    struct Point {
        Vector3 mPos;   /*!< Camera-space position of the point. */
        float mFrame;   /*!< Song position of the point, in frames. */
        float mLane;    /*!< Position across the track, 0 through 1. */
        int mGem;       /*!< Non-zero for a point at a gem, zero for a filler point. */
        int mUnknown1c; /*!< Never written. AddPoint() copies the word from uninitialised stack. */
    };

    /**
     * Resolve the string and the glow system, show the string, and empty it.
     *
     * @param nIndex The player index, selecting "gem_glow<n>.ps".
     * @param colorName The player's colour name, whose first letter selects the string.
     * @ghidraAddress 0x00438518
     */
    TnlSabreTrail(int nIndex, HxStr colorName);

    /**
     * Empty the line.
     *
     * @ghidraAddress 0x00454c50
     */
    ~TnlSabreTrail();

    /**
     * Show or hide the string.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00454d30
     */
    void SetShowing(int nShowing);

    /**
     * Release every glow particle, drop every point, and zero the track and the bar range.
     *
     * @ghidraAddress 0x00454dc8
     */
    void Clear();

    /**
     * Build the line again from the track and bar range it last built.
     *
     * Does nothing while mBarCount is zero.
     *
     * @ghidraAddress 0x00454d60
     */
    void Rebuild();

    /**
     * Build the line through the gems of nBarCount bars of one track.
     *
     * Returns at once when the arguments match the current line and it has points. Each gem gives
     * a point at `tick + bar * 1920` frames and at `0.315 * lane + 0.185` across the track. The
     * gems come from the level's TrackData for nTrack.
     *
     * @param nTrack The tunnel track and the level track.
     * @param nFirstBar The first bar.
     * @param nBarCount The number of bars.
     * @ghidraAddress 0x004387b0
     */
    void Build(int nTrack, int nFirstBar, int nBarCount);

    /**
     * Start an oscillation of the revealed filler points.
     *
     * Ignored with fewer than two points or when flFrame is before the first bar. The strength
     * becomes `nStrength * 0.05`, clamped to 0 through 1, and sets the amplitude, the frequency,
     * and the glow size.
     *
     * @param flFrame The song position the oscillation starts at.
     * @param nStrength The strength, the gems caught so far in the phrase.
     * @param nTotal The gems the phrase requires. AppTunnel's CatchMsg handler loads it into $a2
     *               at `0x00447af4`, and the body does not read it.
     * @ghidraAddress 0x00438ad0
     */
    void Pulse(float flFrame, int nStrength, int nTotal);

    /**
     * Reveal points, advance the oscillation, and shrink the glows toward their floor.
     *
     * Does nothing with fewer than two points.
     *
     * @param flFrame The song position.
     * @ghidraAddress 0x00438c20
     */
    void Update(float flFrame);

private:
    // Place one point on the track and append it, allocating a glow particle for a gem point.
    // 0x004389a8.
    void AddPoint(float flFrame, float flLane, int nGem);

    // Append a gem point, first filling the gap from the last point with a filler point every 60
    // frames on the straight line between the two. 0x00454e48.
    void AddSegmentPoint(float flFrame, float flLane);

    // Decay the amplitude and move every revealed filler point past mPulseFrame. The argument is
    // passed and never read. 0x00438e00.
    void Wobble(float flFrame);

    std::vector<Point> mPoints;
    int mRevealed; // Points handed to mString so far.
    Rnd::String *mString;
    int mFirstBar;
    int mBarCount;
    int mTrack;
    int mIndex;
    float mStartFrame; // Frame of the first Update() after a build.
    float mAmplitude;
    float mStrength;
    float mPulseFrame;
    float mPhase; // Update() count, the time base of the oscillation.
    float mGlowFloor;
    float mGlowSize;
    Rnd::ParticleSys *mGlow;
    int mUnknown44; // +0x44, never accessed by the recovered routines.
};
