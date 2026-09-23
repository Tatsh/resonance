#pragma once

#include "game/powerup.h"

/**
 * Powerup that bumps the other players off their tracks.
 *
 * `11BumpPowerup` in the RTTI descriptor at `0x008ef6a0`, with Powerup as its one base. The factory
 * builds it for kHudItemBumper with a four-byte allocation and the table at `0x007e4090`. The
 * destructor at `0x001c9d28` is implicitly declared.
 */
class BumpPowerup : public Powerup {
public:
    /**
     * Send a BumpPacket to every other game system.
     *
     * The packet carries the player, the bar at `+0x1c`, the track at `+0x20`, and a result at
     * `+0x24` that starts at zero. It goes out through the player's MsgSource. A non-zero result
     * plays `SND_DEPLOY_BUMPER`, and a zero one is followed by a PowerupFailedMsg. The body is not
     * written, because BumpPacket has no payload constructor and its result word is unnamed.
     *
     * @param nTrack The track.
     * @param nBar The bar.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return The packet's result.
     * @ghidraAddress 0x001c9de0
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return kHudItemBumper.
     * @ghidraAddress 0x001c9dd8
     */
    virtual int Type();
};
