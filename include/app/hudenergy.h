#pragma once

namespace Rnd {
class Animatable;
class Mesh;
} // namespace Rnd

/**
 * Energy bar of the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `HUD1 energy` objects it resolves. HudTrack embeds one at
 * `+0x00`. Every track display resolves the same three `HUD1` objects, so all of them drive one
 * shared bar.
 */
class HudEnergy {
public:
    /**
     * Resolve the bar and its animation, and empty it.
     *
     * The frame of `HUD1 energy.mesh` shows only in kPlayModeGame with kGameModeSolo. The level
     * starts at 0, and the constructor then runs SetFrame() 100 times with a frame of -1.
     *
     * @param nIndex The track display number. The body does not read it.
     * @ghidraAddress 0x00416428
     */
    HudEnergy(int nIndex);

    /**
     * Show the level, blinking the bar while the level is low.
     *
     * The animation takes the level scaled to 0 through 100. A level below 0.01 hides the bar, a
     * level below 0.2 shows it for the first 120 of every 240 ticks, and any other level shows it.
     *
     * @param flFrame The song position, in MIDI ticks, that times the blink.
     * @ghidraAddress 0x004166b8
     */
    void SetFrame(float flFrame);

private:
    Rnd::Animatable *mUnknown00; // +0x00 `HUD1 energy bar.msnm`
    Rnd::Mesh *mUnknown04;       // +0x04 `HUD1 energy bar.mesh`

public:
    /**
     * The level, 0 through 1. +0x08
     *
     * Public because Overlay's JuiceAmountMsg handler at `0x0041f310` writes the juice amount into
     * it directly, and the image has no accessor for it.
     */
    float mUnknown08;
};
