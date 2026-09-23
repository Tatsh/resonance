#pragma once

#include "app/linearramp.h"

class HxStr;

namespace Rnd {
class Animatable;
} // namespace Rnd

/**
 * Animation of the head-up display posed by a ramp.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from its two parts. The head-up display panel embeds two, at `+0x58`
 * over `<layout> assembly.view` and at `+0x74` over `HUD1 label swap.tnm`.
 */
class HudAnimRamp {
public:
    /**
     * Resolve the animation, rewind it to frame 0, and set the ramp's range.
     *
     * The panel inlines the body twice, and this copy has no caller.
     *
     * @param name The object name of the animation.
     * @param flFrom The frame at raw ramp position 0.
     * @param flTo The frame at raw ramp position 1.
     * @param flDuration The time the ramp takes to cover the range.
     * @ghidraAddress 0x0042a888
     */
    HudAnimRamp(const HxStr &name, float flFrom, float flTo, float flDuration);

    /**
     * Start the animation moving toward a raw ramp position.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param flTarget The raw target.
     * @ghidraAddress 0x0042a958
     */
    void SetTarget(float flTarget);

    /**
     * Move the animation to a raw ramp position at once.
     *
     * The out-of-line copy has no caller. The title is inferred.
     *
     * @param flTarget The raw target.
     * @ghidraAddress 0x0042a978
     */
    void Jump(float flTarget);

    /**
     * Advance the ramp and pose the animation on its value while it moves.
     *
     * The panel inlines the body, and this copy has no caller.
     *
     * @param flTime The time the ramp runs against.
     * @ghidraAddress 0x0042a998
     */
    void Update(float flTime);

private:
    Rnd::Animatable *mAnim;
    LinearRamp mRamp;
};
