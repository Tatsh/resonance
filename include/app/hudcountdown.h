#pragma once

namespace Rnd {
class Blur;
class Text;
class TransAnim;
} // namespace Rnd

/**
 * Bar countdown of one player's track display on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `countdown` objects it resolves. HudTrack embeds one at
 * `+0xc4`.
 *
 * The countdown shows the number of bars remaining before a target bar, and only while that number
 * is 1 through 9. It runs only in kPlayModeGame, and not at all when configuration code 0x3a1 is
 * set.
 */
class HudCountdown {
public:
    /**
     * Resolve the countdown's objects and hide its blur.
     *
     * @param nIndex The track display number that fills `<layout> countdown<n>`.
     * @param nTargetBar The bar the countdown runs to. HudTrack's constructor reads it from
     *        PlayMap::Slot9() of Globals::GetPlayMap(), the last bar of the level.
     * @ghidraAddress 0x004191b8
     */
    HudCountdown(int nIndex, int nTargetBar);

    /**
     * Show the bars remaining and advance the digit animation.
     *
     * A change of count sets the text and restarts the animation from the current frame.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x004194c0
     */
    void SetFrame(float flFrame);

private:
    int mTargetBar;
    // The count last shown. Starts at -123.
    int mShownCount;
    // The frame the count last changed at. Starts at 1e9.
    float mChangeFrame;
    Rnd::TransAnim *mAnim; // `<layout> countdown<n>.tnm`
    Rnd::Text *mText;      // `<layout> countdown<n>.txt`
    Rnd::Blur *mBlur;      // `<layout> countdown<n>.blur`
    // Non-zero when the countdown never runs.
    int mDisabled;
};
