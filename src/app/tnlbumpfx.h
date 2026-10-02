#pragma once

#include "os/hxstr.h"

namespace Rnd {
class Generator;
class Mat;
class TransAnim;
} // namespace Rnd

/**
 * Burst of generated instances that sweeps along a short stretch of the tunnel.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the three objects it drives, "bumpfx<n>.mgen", "bumpfx<n>.mat",
 * and "bumpfx<n>.tnm".
 *
 * A burst runs in two timed phases once it is started. For the first 500 frames the generator
 * spawns with its authored rate while its path window follows the song, 800 frames long and
 * running forward or backward. The spawn rate then drops to one instance per 1e9 frames, and 1500
 * frames after the start the generator hides.
 *
 * AppTunnel allocates these in a loop, 0x20 bytes each, with the loop index as nIndex.
 */
class TnlBumpFX {
public:
    /**
     * Resolve the objects of one burst slot and silence its generator.
     *
     * The generator's low spawn rate is recorded before both rate bounds are set to 1e9, and the
     * generator is hidden.
     *
     * @param nIndex The burst slot.
     * @ghidraAddress 0x0043dc20
     */
    explicit TnlBumpFX(int nIndex);

    /**
     * Give the generator back both rate bounds at the recorded low rate.
     *
     * @ghidraAddress 0x00456690
     */
    ~TnlBumpFX();

    /**
     * Start a burst that sweeps towards one lane in one player's colour.
     *
     * The generator gets its authored spawn rate back, and its spawn clock restarts. The first
     * rotation key of the path's frame owner becomes a turn about Y of `(1 - nStep / 8)` turns,
     * the rotation keys are sorted, and every rotation tangent is rebuilt. The material's emissive
     * colour becomes the player's colour, and the generator shows.
     *
     * @param nStep The lane, in eighths of a turn.
     * @param colorName The player's colour name. The callers pass a copy, and the name reaches
     *                  TnlColorFromName() with no further copy.
     * @param nForward Non-zero runs the path window forward.
     * @param flPathOffset The distance from the song frame to the path window.
     * @ghidraAddress 0x0043dea8
     */
    void Start(int nStep, const HxStr &colorName, int nForward, float flPathOffset);

    /**
     * Report whether the burst waits for Start().
     *
     * AppTunnel's routine at `0x00457828` tests the state inline to find a free burst, and no
     * out-of-line copy exists.
     *
     * @return True while idle.
     */
    bool IsIdle() const {
        return mState == kStateIdle;
    }

    /**
     * Advance the burst.
     *
     * While idle the routine only records flFrame. Otherwise the generator is driven to the time
     * since the start. During the first phase the path window is reset to begin at
     * `flFrame + mPathOffset` and to end 800 frames later, or earlier when mForward is clear.
     *
     * @param flFrame The current frame.
     * @ghidraAddress 0x004566c8
     */
    void SetFrame(float flFrame);

private:
    // Phase SetFrame() runs. Spawning follows a start and lasts 500 frames, and draining ends
    // 1500 frames after the start by hiding the generator.
    enum State { kStateIdle = 0, kStateSpawning = 1, kStateDraining = 2 };

    Rnd::Generator *mGenerator; // "bumpfx<n>.mgen".
    Rnd::TransAnim *mTransAnim; // "bumpfx<n>.tnm".
    Rnd::Mat *mMat;             // "bumpfx<n>.mat".
    float mPathOffset;          // Distance from the song frame to the path window.
    int mForward;               // Non-zero runs the path window forward.
    State mState;
    float mStartFrame;   // Last frame SetFrame() received while idle.
    float mSavedRateGen; // The generator's authored low spawn rate.
};
