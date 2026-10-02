#pragma once

#include "game/powerup.h"

/**
 * Powerup that multiplies the player's score for a stretch of bars.
 *
 * `17MultiplierPowerup` in the RTTI descriptor at `0x008f08b0`, with Powerup as its one base. The
 * factory builds it for kHudItemMultiplier with a four-byte allocation and the table at
 * `0x007e4018`. The destructor at `0x001ca160` is implicitly declared.
 */
class MultiplierPowerup : public Powerup {
public:
    /**
     * Start the multiplier bonus.
     *
     * A MultiplierMsg (player at `+0x04`, bar at `+0x08`, and 4 at `+0x0c`) is delivered straight
     * to the player's sink, and `SND_DEPLOY_MULTIPLIER` plays.
     *
     * @param nTrack Not read.
     * @param nBar The bar the bonus starts at.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return 1 always.
     * @ghidraAddress 0x001ca218
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return kHudItemMultiplier.
     * @ghidraAddress 0x001ca210
     */
    virtual int Type();
};
