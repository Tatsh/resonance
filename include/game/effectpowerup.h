#pragma once

#include "game/powerup.h"

/**
 * Powerup that toggles one jam effect on a track.
 *
 * `13EffectPowerup` in the RTTI descriptor at `0x008efb50`, with Powerup as its one base. The
 * factory's shared arm builds it for the six effect kinds, kHudItemVolume through kHudItemChorus,
 * with an eight-byte allocation and the table at `0x007e4068`. The destructor at `0x001c9f08` is
 * implicitly declared.
 */
class EffectPowerup : public Powerup {
public:
    /**
     * Record the kind this powerup reports.
     *
     * Inline. CreateForType() expands it and stores the kind after the table.
     *
     * @param nEffectType The kind, a HudItemKind from kHudItemVolume through kHudItemChorus.
     */
    explicit EffectPowerup(int nEffectType) : mEffectType(nEffectType) {
    }

    /**
     * Ask the track's JamEffectsMgr to toggle the effect for one bar.
     *
     * A JamEffectMsg (bar at `+0x04`, track at `+0x08`, mEffectType at `+0x0c`, player at
     * `+0x10`) goes out through the player's MsgSource. The body is not written, because
     * JamEffectMsg's payload is private.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return 1 always.
     * @ghidraAddress 0x001c9fc0
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return mEffectType.
     * @ghidraAddress 0x001c9fb8
     */
    virtual int Type();

private:
    int mEffectType; // +0x04
};
