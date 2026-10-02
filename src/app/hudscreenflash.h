#pragma once

namespace Rnd {
class Mesh;
} // namespace Rnd

/**
 * Full-screen fade of the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from `HUD screen rect`, the one object it resolves. The head-up display
 * panel embeds one at `+0x40`.
 *
 * The fade is a black rectangle over the whole screen whose vertex alpha runs between 0 and 1 over
 * a duration, timed by the free-running clock rather than by the song position.
 */
class HudScreenFlash {
public:
    /**
     * Resolve the rectangle and show it fully opaque.
     *
     * @ghidraAddress 0x0041b150
     */
    HudScreenFlash();

    /**
     * Start a fade.
     *
     * A duration below 1 is raised to 1. Overlay's FadeGameMsg handler is the caller. The title is
     * inferred.
     *
     * @param flDuration The time the fade takes, in milliseconds.
     * @param nFadeIn Non-zero to fade from opaque to clear, and zero to fade from clear to opaque.
     * @ghidraAddress 0x0042a548
     */
    void Start(float flDuration, int nFadeIn);

    /**
     * Set the rectangle's alpha for the current time and show it.
     *
     * Does nothing before the fade starts. Once the fade has run its duration, the alpha stays at
     * its end value and the fade is marked finished.
     *
     * @ghidraAddress 0x0041b2a0
     */
    void SetFrame();

private:
    // When the fade started, in the units of mScale. 1e9 marks no fade in progress.
    float mStart;
    // Share of the fade covered per unit of time.
    float mRate;
    Rnd::Mesh *mMesh; // `HUD screen rect`
    // Milliseconds per unit of GetElapsedMilliseconds().
    float mScale;
    // Non-zero while the fade runs from opaque to clear.
    int mFadeIn;
};
