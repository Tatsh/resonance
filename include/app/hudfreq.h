#pragma once

class Player;

namespace Rnd {
class Mat;
class MatAnim;
class Mesh;
class TransAnim;
} // namespace Rnd

/**
 * FreQ icon of one player's badge on the head-up display, with a pulse that marks the player.
 *
 * The class is not polymorphic, emits no RTTI, and is never allocated on its own. No descriptor,
 * allocation tag, or file path identifies it, and its name is inferred from the `%s freq%d` family
 * of objects its constructor resolves. It occupies `+0x10` to `+0x27` of HudBadge.
 */
class HudFreq {
public:
    /**
     * Resolve the icon's objects for one player.
     *
     * Resolves `<layout> freq<p>.tnm`, `HUD freq pulse.mnm`, `HUD freq<p>.mat`, and
     * `<layout> freq<n>.mesh`, where `<p>` is the player's identifier. Sets the material's emissive
     * colour to HudColorFromName() of the player's colour name, gives its second stage the persona
     * burn texture for the badge number, and puts it on the mesh. The mesh is shown except in a
     * jukebox session. The icon starts without its pulse.
     *
     * @param pPlayer The player the icon shows.
     * @param nIndex The badge number that fills `<n>` and selects the persona burn texture.
     * @ghidraAddress 0x00419b40
     */
    HudFreq(Player *pPlayer, int nIndex);

    /**
     * Start or stop the icon's pulse.
     *
     * Overlay::OnLeaderChanged() pulses the leader's icon, and Overlay's JuiceAmountMsg handler
     * pulses the icon of a solo player whose juice passes 0.85. Both write the word through an
     * inlined copy of this routine. The title is inferred.
     *
     * @param nPulsing Non-zero to pulse.
     * @ghidraAddress 0x0042a348
     */
    void SetPulsing(int nPulsing);

    /**
     * Advance the icon animation and the pulse.
     *
     * The icon animation follows the frame for its first 500 ticks. The pulse loops every 480
     * ticks, 80 ticks ahead of the frame, and restarts its loop count only while pulsing is on.
     * HudBadge::SetFrame() inlines the body, and this copy has no caller.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x0042a350
     */
    void SetFrame(float flFrame);

private:
    Rnd::TransAnim *mAnim;    // `<layout> freq<p>.tnm`
    Rnd::Mesh *mMesh;         // `<layout> freq<n>.mesh`
    Rnd::MatAnim *mPulseAnim; // `HUD freq pulse.mnm`
    Rnd::Mat *mMat;           // `HUD freq<p>.mat`
    // Non-zero while the icon pulses.
    int mPulsing;
    // The pulse loop the frame is in. Starts at -100.
    int mPulseLoop;
};
