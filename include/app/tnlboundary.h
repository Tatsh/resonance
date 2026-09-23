#pragma once

class PlayMap;
namespace Rnd {
class Text;
class View;
} // namespace Rnd

/**
 * Marker that stands in the tunnel at the next step boundary and labels it.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the two objects it drives, "boundary.view" and "boundary msg".
 *
 * The view sits on the tunnel path at the start bar of mStep and animates against the distance to
 * that bar. Once the song is more than a quarter bar past it, the marker moves on to the next
 * step. In game mode the message reads "START" at the first section, "FINAL\nSECTION" at the last,
 * and "FINISH" one past the last. It is empty everywhere else and outside game mode.
 *
 * AppTunnel allocates one, 0x14 bytes, and stores it at `+0x10`.
 */
class TnlBoundary {
public:
    /**
     * Place the marker at bar 0 and write its message.
     *
     * The message only shows in game mode.
     *
     * @param pPlayMap The play map whose steps the marker walks.
     * @ghidraAddress 0x0043fa18
     */
    explicit TnlBoundary(PlayMap *pPlayMap);

    /**
     * Animate the marker and advance it past a boundary the song has passed.
     *
     * The view is driven to `flFrame - mStep * 1920 + 2000`. When flFrame is more than 480 frames
     * past the start of mStep, mStep becomes PlayMap::FollowingStepBar() of it, the view moves to
     * the path at the new bar, and the message is rewritten.
     *
     * @param flFrame The current frame.
     * @ghidraAddress 0x0043fcc0
     */
    void SetFrame(float flFrame);

    /**
     * Write the message for mStep into the "boundary msg" text.
     *
     * AppTunnel's handler for an AdvanceSectionToggleMsg calls it as well as SetFrame().
     *
     * @ghidraAddress 0x0043fdf0
     */
    void UpdateText();

private:
    PlayMap *mPlayMap;
    Rnd::View *mView; // "boundary.view".
    Rnd::Text *mText; // "boundary msg".
    int mStep;        // Bar the marker stands at.
    int mStepCount;   // PlayMap::Slot10() at construction.
};
