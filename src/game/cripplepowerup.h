#pragma once

#include "game/powerup.h"

/**
 * Powerup that cripples the other players.
 *
 * Its RTTI descriptor is at `0x008f08a0`. It has Powerup as its one base. The factory builds it for
 * kHudItemCrippler with a four-byte allocation and the table at `0x007e4108`. The destructor at
 * `0x001c9778` is implicitly declared.
 */
class CripplePowerup : public Powerup {
public:
    /**
     * Ask the game to cripple the other players.
     *
     * A CrippleMsg (player at `+0x08`, track at `+0x0c`, bar at `+0x10`) goes out through the
     * player's MsgSource. A handled message is followed by a DeployedPowerupMsg with no bar range
     * and a track of -1, and `SND_DEPLOY_CRIPPLER` plays. An unhandled one is followed by a
     * PowerupFailedMsg.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return The message's result.
     * @ghidraAddress NTSC-U/C: 0x001c9830
     * @ghidraAddress PAL: 0x001cf6d0
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return kHudItemCrippler.
     * @ghidraAddress NTSC-U/C: 0x001c9828
     * @ghidraAddress PAL: 0x001cf6c8
     */
    virtual int Type();
};
