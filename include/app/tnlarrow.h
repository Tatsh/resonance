#pragma once

class TnlPlayer;
namespace Rnd {
class Mesh;
class View;
} // namespace Rnd

/**
 * Arrow mesh that appears in the local view of up to four players for a limited time.
 *
 * The class is not polymorphic and emits no RTTI, and no allocation tag or file path identifies
 * it. The name is inferred from the one object it drives, "arrow<n>.mesh".
 *
 * The same mesh is drawn by the view of each player slot it is shown to, until the frame recorded
 * for that slot passes.
 *
 * AppTunnel allocates these in a loop, 0x24 bytes each, with the loop index as nIndex.
 */
class TnlArrow {
public:
    /** Number of player slots the arrow can be shown to. */
    static constexpr int kSlotCount = 4;

    /**
     * Resolve "arrow<nIndex>.mesh" with every slot hidden.
     *
     * @param nIndex The arrow.
     * @ghidraAddress 0x0043ff10
     */
    explicit TnlArrow(int nIndex);

    /**
     * Hide the arrow from every slot.
     *
     * @ghidraAddress 0x00456e20
     */
    ~TnlArrow();

    /**
     * Show the arrow in the local view of one player until a frame.
     *
     * The slot is the player number less 1. A slot that already shows the arrow is hidden first.
     * AppTunnel's message handler inlines this at `0x004495e8`, and the out-of-line copy has no
     * caller.
     *
     * @param pPlayer The player.
     * @param flExpireFrame The frame after which the slot hides.
     * @ghidraAddress 0x00456e90
     */
    void Show(TnlPlayer *pPlayer, float flExpireFrame);

    /**
     * Take the arrow out of the view of one slot.
     *
     * A slot the arrow is not shown to is unchanged. The expiry frame of the slot is not reset.
     *
     * @param nSlot The player slot, from 0.
     * @ghidraAddress 0x00456f20
     */
    void Hide(int nSlot);

    /**
     * Hide the arrow from every slot whose expiry frame is before flFrame.
     *
     * The expiry frame of such a slot returns to 1e9.
     *
     * @param flFrame The current frame.
     * @ghidraAddress 0x00456f68
     */
    void SetFrame(float flFrame);

private:
    Rnd::Mesh *mMesh;
    Rnd::View *mViews[kSlotCount];   // View drawing the arrow for each slot, or null.
    float mExpireFrames[kSlotCount]; // Frame after which each slot hides, 1e9 when hidden.
};
