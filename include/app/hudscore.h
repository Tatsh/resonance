#pragma once

class Player;

namespace Rnd {
class Mesh;
class Text;
} // namespace Rnd

/**
 * Score readout of one player's badge on the head-up display.
 *
 * The class is not polymorphic, emits no RTTI, and is never allocated on its own. No descriptor,
 * allocation tag, or file path identifies it, and its name is inferred from the `%s score%d` family
 * of objects its constructor resolves. It occupies the first 0x10 bytes of HudBadge.
 *
 * The readout redraws once more than 600 units of Update()'s time have passed since a change.
 * mUnknown08 records when the change arrived, -1 marks a change whose time is not yet recorded, and
 * 1e9 marks the readout as up to date.
 *
 * The constructor's body is not written. It needs the Rnd::Font material setter at `0x004d0600`.
 */
class HudScore {
public:
    /**
     * Resolve the readout's objects for one player.
     *
     * Resolves `<layout> score<n>.mesh`, `.txt`, and `.font`, where `<layout>` is g_hudLayoutName,
     * and gives the font the material `HUD score font <colour>.mat` for the player's colour name.
     * Shows the mesh only in play mode 1, and starts the pending time at -100000 with a score of
     * 0, which makes the first Update() redraw at once.
     *
     * @param pPlayer The player whose colour the font takes.
     * @param nIndex The badge number that fills `<n>`.
     * @ghidraAddress 0x00419618
     */
    HudScore(Player *pPlayer, int nIndex);

    /**
     * Redraw the score once the change has settled.
     *
     * HudBadge::SetFrame() inlines the body, and this copy has no caller.
     *
     * @param flTime The time HudBadge::SetFrame() receives as its second argument.
     * @ghidraAddress 0x0042a220
     */
    void Update(float flTime);

    /**
     * The readout's backing mesh. +0x00
     *
     * Public because HudScorePulse::MoveTo() reads its world position directly, and the image has
     * no accessor for it.
     */
    Rnd::Mesh *mUnknown00;

private:
    Rnd::Text *mUnknown04; // +0x04
    // When the pending change arrived. See the class documentation for the two sentinels.
    float mUnknown08; // +0x08
    // The score the text shows once Update() redraws.
    int mUnknown0c; // +0x0c
};
