#pragma once

#include "app/animrange.h"

namespace Rnd {
class Blur;
class Mat;
class Text;
class View;
} // namespace Rnd

/**
 * Points readout of one player's track display on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `pts` objects it resolves. HudTrack embeds one at `+0x78`.
 *
 * The readout shows the points a phrase is building, a multiplier beside them, and an exit
 * animation that moves banked points away. The text pulses and flashes on each change and
 * settles back over the following frames.
 */
class HudPoints {
public:
    /**
     * Resolve the readout's objects, arm the exit animation at frame 100, and hide both texts.
     *
     * @param nIndex The track display number that fills `<layout> pts<n>`, `ptsmult<n>`, and
     *        `pts_exit<n>`.
     * @ghidraAddress 0x00418818
     */
    HudPoints(int nIndex);

    /**
     * Advance the exit animation, apply the pulse and flash, and let both decay.
     *
     * The points text is scaled horizontally and in depth by the pulse and brightened by the
     * flash, the multiplier text takes the colour of the plain or the hot material, and the flash
     * and the pulse then fall by 0.2 and 0.05.
     *
     * @param flTime The time AnimRange::Update() compares against.
     * @ghidraAddress 0x00418de8
     */
    void SetFrame(float flTime);

    /**
     * Show banked points leaving the readout.
     *
     * Hides the points text, sets the exit text, and plays the exit animation from frame 0 to
     * frame 100 at full flash. The title is inferred.
     *
     * @param nPoints The points to show leaving.
     * @ghidraAddress 0x00418f90
     */
    void ShowExit(int nPoints);

    /**
     * Send the points shown out through the exit animation, if any are shown, and hide the text.
     *
     * The exit animation plays from frame 200 to frame 300. The title is inferred.
     *
     * @ghidraAddress 0x004190a8
     */
    void Bank();

    /**
     * Show a points total and start its flash from zero.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param nPoints The points to show.
     * @ghidraAddress 0x00429fe0
     */
    void SetPoints(int nPoints);

    /**
     * Show a multiplier as `x<n>`, visible only above 1.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param nMultiplier The multiplier.
     * @ghidraAddress 0x0042a0c8
     */
    void SetMultiplier(int nMultiplier);

private:
    // The flash, 1 at a change and falling by 0.2 each frame to 0.
    float mFlash;
    // The pulse, falling by 0.05 each frame to mPulseRest.
    float mPulse;
    float mPulseRest;
    // The multiplier SetMultiplier() last showed. Starts at 1.
    int mMultiplier;
    // The points SetPoints() last showed.
    int mPoints;
    // Non-zero while points are shown.
    int mShowing;
    // Selects the hot material for the multiplier text.
    int mHot;
    Rnd::View *mExitView;       // `pts_exit<n>.view`
    Rnd::Text *mExitText;       // `pts_exit<n>.txt`
    Rnd::Blur *mExitBlur;       // `pts_exit<n>.blur`
    Rnd::Text *mPointsText;     // `pts<n>.txt`
    Rnd::Text *mMultiplierText; // `ptsmult<n>.txt`
    Rnd::Mat *mPlainMat;        // `HUD ptstmp.mat`
    Rnd::Mat *mHotMat;          // `HUD ptstmphot.mat`
    int mUnknown38;             // +0x38
    // The exit animation.
    AnimRange mExit;
};
