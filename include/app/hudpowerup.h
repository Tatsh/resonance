#pragma once

namespace Rnd {
class View;
} // namespace Rnd

/**
 * Powerup indicator of one player's track display on the head-up display.
 *
 * The class is not polymorphic and emits no RTTI. No descriptor, tag, or file path identifies it,
 * and its name is inferred from the `pup` objects it resolves. HudTrack embeds one at `+0x0c`.
 *
 * The indicator is one container view, `<layout> pup<n>.view`, whose scene Show() refills with the
 * view of one powerup kind. The six kinds with a view are the five powerups and the multiplier.
 */
class HudPowerup {
public:
    /**
     * Resolve the container and the six powerup views, and show nothing.
     *
     * Shows the indicator's mesh, `<layout> pup<n>.mesh`, only in kPlayModeGame.
     *
     * @param nIndex The track display number that fills `<n>` and the views' `%d`.
     * @ghidraAddress 0x004167d0
     */
    HudPowerup(int nIndex);

    /**
     * Show the view of one item kind, or nothing.
     *
     * Empties the container's draw, transform, and animation lists, then adds the kind's view to
     * all three after showing it. For a kind with no view, the container stays empty. The title is
     * inferred.
     *
     * @param nKind The HudItemKind to show, or kHudItemNone.
     * @ghidraAddress 0x004299a0
     */
    void Show(int nKind);

private:
    Rnd::View *mUnknown00; // +0x00 `HUD pup auto<n>.view`
    Rnd::View *mUnknown04; // +0x04 `HUD pup neut<n>.view`
    Rnd::View *mUnknown08; // +0x08 `HUD pup bump<n>.view`
    Rnd::View *mUnknown0c; // +0x0c `HUD pup crip<n>.view`
    Rnd::View *mUnknown10; // +0x10 `HUD pup free<n>.view`
    Rnd::View *mUnknown14; // +0x14 `HUD pup mult<n>.view`
    Rnd::View *mUnknown18; // +0x18 The container, `<layout> pup<n>.view`.
};
