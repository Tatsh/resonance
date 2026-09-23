#pragma once

#include "game/powerup.h"

/**
 * Powerup that lets a player freestyle over the song.
 *
 * `16FreestylePowerup` in the RTTI descriptor at `0x008eee88`, with Powerup as its one base. The
 * factory builds it for kHudItemFreestyler with a four-byte allocation and the table at
 * `0x007e40e0`. The destructor at `0x001c9970` is implicitly declared.
 */
class FreestylePowerup : public Powerup {
public:
    /**
     * Enable freestyle for the player from a bar.
     *
     * Nothing happens and zero is reported while Player::Slot5() is non-zero. Otherwise an
     * EnableFreestyleMsg goes out through the player's MsgSource. A handled message is followed by
     * a DeployedPowerupMsg with no bar range and a track of -1, and `SND_DEPLOY_FREESTYLER` plays.
     * An unhandled one is followed by a PowerupFailedMsg.
     *
     * @param nTrack Not read.
     * @param nBar The bar freestyle starts at.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return The message's result, non-zero when freestyle was enabled.
     * @ghidraAddress 0x001c9a28
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return kHudItemFreestyler.
     * @ghidraAddress 0x001c9a20
     */
    virtual int Type();
};
