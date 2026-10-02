#pragma once

class Player;

/**
 * Policy that decides which tracks a player may catch at each bar.
 *
 * `9EnableMgr` in the RTTI descriptor at `0x0086f750`, a root class with no data beyond its vptr.
 * Its type function is at `0x00105060` and its table at `0x007ccc68` has six entries. Slots 2 and 5
 * are pure. Gamer owns one at `+0x90`, builds it for the session mode, and forwards
 * Gamer::SetBarOwner() to slot 2 and Gamer::QueryBar() to slot 5. GameEnableMgr,
 * LocalJamEnableMgr, and NetJamEnableMgr derive from it.
 *
 * Every slot title is inferred from the overrides and from Gamer's calls.
 */
class EnableMgr {
public:
    /** @ghidraAddress 0x001050a0 */
    virtual ~EnableMgr();

    /**
     * Record the player who owns one bar of one track.
     *
     * Slot 2, pure.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param pPlayer The owner, or the null player.
     */
    virtual void SetBarOwner(int nTrack, int nBar, Player *pPlayer) = 0;

    /**
     * Leave one track free of its requirements until a bar.
     *
     * Slot 3. The base implementation does nothing. Gamer passes the bar a player captured and
     * the bar eight later.
     *
     * @param nTrack The track.
     * @param nBar The current bar.
     * @param nUntilBar The first bar the requirements apply to again.
     * @ghidraAddress 0x001050d0
     */
    virtual void SetFreeUntil(int nTrack, int nBar, int nUntilBar);

    /**
     * Stop one track from ever being enabled.
     *
     * Slot 4. The base implementation does nothing. Gamer runs it for every track outside catch
     * mode.
     *
     * @param nTrack The track.
     * @ghidraAddress 0x001050d8
     */
    virtual void DisableTrack(int nTrack);

    /**
     * Report whether one bar of one track may be caught.
     *
     * Slot 5, pure.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @return Non-zero when the bar may be caught.
     */
    virtual int QueryBar(int nTrack, int nBar) = 0;
};
