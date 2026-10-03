#pragma once

namespace Rnd {
class Mesh;
} // namespace Rnd

/**
 * Loop indicator of one player's track display on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `loop` objects it resolves. HudTrack embeds one at `+0x48`.
 */
class HudLoop {
public:
    /**
     * Resolve the indicator and its wires, and show the indicator.
     *
     * The wires, `<layout> loopwires<n>.mesh`, show only in kPlayModeJam.
     *
     * @param nIndex The track display number that fills `<n>`.
     * @ghidraAddress NTSC-U/C: 0x00417b40
     * @ghidraAddress PAL: 0x00451b00
     */
    HudLoop(int nIndex);

    /**
     * Show or hide the indicator, `<layout> loop<n>.mesh`.
     *
     * The constructor is its one caller. The name is inferred.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress NTSC-U/C: 0x00429e38
     * @ghidraAddress PAL: 0x00465478
     */
    void SetShowing(int nShowing);

private:
    Rnd::Mesh *mIndicator; // `<layout> loop<n>.mesh`
    Rnd::Mesh *mWires;     // `<layout> loopwires<n>.mesh`
};
