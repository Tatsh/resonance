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
     * @ghidraAddress 0x00417b40
     */
    HudLoop(int nIndex);

    /**
     * Show or hide the indicator, `<layout> loop<n>.mesh`.
     *
     * The routine is titled `UnidentifiedBody00429d30Copy2` in the program, which files it as a
     * duplicate emission. The constructor is its one caller. The title is inferred.
     *
     * @param nShowing Non-zero to show.
     * @ghidraAddress 0x00429e38
     */
    void SetShowing(int nShowing);

private:
    Rnd::Mesh *mUnknown00; // +0x00 `<layout> loop<n>.mesh`
    Rnd::Mesh *mUnknown04; // +0x04 `<layout> loopwires<n>.mesh`
};
