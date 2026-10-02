#pragma once

#include "app/linearramp.h"

namespace Rnd {
class View;
} // namespace Rnd

/**
 * Letterbox bars of the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the one object it resolves. The head-up display panel embeds one at
 * `+0x118`.
 *
 * The bars are one view whose animation a ramp drives from frame 0 to frame 100 over 480 units of
 * time. The view shows only while the ramp is off frame 0.
 */
class HudLetterbox {
public:
    /**
     * Resolve the view, hide it, and set the ramp's range.
     *
     * @ghidraAddress 0x0041b3b0
     */
    HudLetterbox();

    /**
     * Advance the ramp, pose the view on its value, and show the view unless the value is 0.
     *
     * The head-up display panel inlines the body, and this copy has no caller.
     *
     * @param flTime The time the ramp runs against.
     * @ghidraAddress 0x0042a608
     */
    void SetFrame(float flTime);

    /**
     * Start the bars moving toward a raw ramp position.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param flTarget The raw target, 0 for no bars and 1 for full bars.
     * @ghidraAddress 0x0042a698
     */
    void SetTarget(float flTarget);

    /**
     * Move the bars to a raw ramp position at once.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param flTarget The raw target.
     * @ghidraAddress 0x0042a6b8
     */
    void Jump(float flTarget);

private:
    Rnd::View *mView; // `HUD letterbox.view`
    LinearRamp mRamp;
};
