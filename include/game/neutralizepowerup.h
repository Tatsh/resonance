#pragma once

#include "game/powerup.h"

/**
 * Powerup that neutralises the phrases other players captured on a track.
 *
 * `17NeutralizePowerup` in the RTTI descriptor at `0x00901f70`, with Powerup as its one base. The
 * factory builds it for kHudItemNeutralizer with a four-byte allocation and the table at
 * `0x007e40b8`. The destructor at `0x001c9b88` is implicitly declared.
 */
class NeutralizePowerup : public Powerup {
public:
    /**
     * Ask the track's PhraseNeutralizer to clear the bars after nBar.
     *
     * A NeutralizeMsg (bar at `+0x08`, track at `+0x0c`, player at `+0x10`) goes out through the
     * player's MsgSource. A handled message plays `SND_DEPLOY_NEUTRALIZER`, and an unhandled one is
     * followed by a PowerupFailedMsg.
     *
     * @param nTrack The track to neutralise.
     * @param nBar The bar the neutralised bars follow.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return The message's result.
     * @ghidraAddress 0x001c9c40
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return kHudItemNeutralizer.
     * @ghidraAddress 0x001c9c38
     */
    virtual int Type();
};
