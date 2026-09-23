#pragma once

class Player;

namespace Rnd {
class Mat;
class MatAnim;
class Mesh;
class TransAnim;
} // namespace Rnd

/**
 * FreQ icon of one player's badge on the head-up display, with the pulse that marks the leader.
 *
 * The class is not polymorphic, emits no RTTI, and is never allocated on its own. No descriptor,
 * allocation tag, or file path identifies it, and its name is inferred from the `%s freq%d` family
 * of objects its constructor resolves. It occupies `+0x10` to `+0x27` of HudBadge.
 *
 * The constructor's body is not written. It needs the material texture setter at `0x004dd0a0`,
 * Rnd::Mat's colour slot 11, and the Globals accessor at `0x00118e38`. SetFrame() is not written,
 * because it needs the Rnd::MatAnim material setter at `0x004dd2d8`.
 */
class HudFreq {
public:
    /**
     * Resolve the icon's objects for one player.
     *
     * Resolves `<layout> freq<p>.tnm`, `HUD freq pulse.mnm`, `HUD freq<p>.mat`, and
     * `<layout> freq<n>.mesh`, where `<p>` is the player's word at `+0x20`. Tints the material with
     * HudColorFromName() of the player's colour name, gives it the persona burn texture for the
     * badge number, and puts it on the mesh. The mesh is shown unless the Globals accessor at
     * `0x00118e38` reports non-zero. The icon starts as not the leader.
     *
     * @param pPlayer The player the icon shows.
     * @param nIndex The badge number that fills `<n>` and selects the persona burn texture.
     * @ghidraAddress 0x00419b40
     */
    HudFreq(Player *pPlayer, int nIndex);

    /**
     * Mark the icon as the leader's or not.
     *
     * The title is inferred from Overlay::OnLeaderChanged(), which writes the same word directly.
     *
     * @param nLeader Non-zero for the leader.
     * @ghidraAddress 0x0042a348
     */
    void SetLeader(int nLeader);

    /**
     * Advance the icon animation and the leader pulse.
     *
     * The icon animation follows the frame for its first 500 ticks. The pulse loops every 480
     * ticks, 80 ticks ahead of the frame, and restarts its loop count only while the icon is the
     * leader's. HudBadge::SetFrame() inlines the body, and this copy has no caller.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @ghidraAddress 0x0042a350
     */
    void SetFrame(float flFrame);

private:
    Rnd::TransAnim *mUnknown00; // +0x00
    Rnd::Mesh *mUnknown04;      // +0x04
    Rnd::MatAnim *mUnknown08;   // +0x08
    Rnd::Mat *mUnknown0c;       // +0x0c
    // Non-zero while the player leads. Overlay::OnLeaderChanged() writes it through an inlined
    // SetLeader().
    int mUnknown10; // +0x10
    // The pulse loop the frame is in. Starts at -100.
    int mUnknown14; // +0x14
};
