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
     * @param nTargetBar The bar the countdown runs to. HudTrack's constructor reads it from slot 9
     *        of the object the Globals accessor at `0x00118da0` returns.
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
    int mUnknown00;             // +0x00 The target bar.
    int mUnknown04;             // +0x04 The count last shown. Starts at -123.
    float mUnknown08;           // +0x08 The frame the count last changed at. Starts at 1e9.
    Rnd::TransAnim *mUnknown0c; // +0x0c
    Rnd::Text *mUnknown10;      // +0x10
    Rnd::Blur *mUnknown14;      // +0x14
    // Non-zero when the countdown never runs.
    int mUnknown18; // +0x18
};
