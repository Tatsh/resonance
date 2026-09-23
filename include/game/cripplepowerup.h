#pragma once

#include "game/powerup.h"

/**
 * Powerup that cripples the other players.
 *
 * `14CripplePowerup` in the RTTI descriptor at `0x008f08a0`, with Powerup as its one base. The
 * factory builds it for kHudItemCrippler with a four-byte allocation and the table at `0x007e4108`.
 * The destructor at `0x001c9778` is implicitly declared.
 */
class CripplePowerup : public Powerup {
public:
    /**
     * Ask the game to cripple the other players.
     *
     * A CrippleMsg (player at `+0x08`, track at `+0x0c`, bar at `+0x10`) goes out through the
     * player's MsgSource. A handled message is followed by a DeployedPowerupMsg with no bar range
     * and a track of -1, and `SND_DEPLOY_CRIPPLER` plays. An unhandled one is followed by a
     * PowerupFailedMsg. The body is not written, because CrippleMsg's bar at `+0x10` is private.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return The message's result.
     * @ghidraAddress 0x001c9830
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return kHudItemCrippler.
     * @ghidraAddress 0x001c9828
     */
    virtual int Type();
};
