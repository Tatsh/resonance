#pragma once

#include "game/powerup.h"

/**
 * Powerup that catches the next bars of a track automatically.
 *
 * `16AutocatchPowerup` in the RTTI descriptor at `0x00901f60`, with Powerup as its one base. The
 * factory builds it for kHudItemAutocatcher with a four-byte allocation and the table at
 * `0x007e4130`. The destructor at `0x001c94f8` is implicitly declared.
 */
class AutocatchPowerup : public Powerup {
public:
    /**
     * Ask the catchers to play bars for the player.
     *
     * One AutoCatchMsg per bar, from nBar on, goes out through the player's MsgSource: one bar when
     * Player::Slot19() reports 1 and four otherwise. When any catcher marked a message handled, a
     * DeployedPowerupMsg covering the bars follows and `SND_DEPLOY_AUTOCATCHER` plays. Otherwise a
     * PowerupFailedMsg goes out.
     *
     * @param nTrack The track.
     * @param nBar The first bar.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return Non-zero when a catcher handled a bar.
     * @ghidraAddress 0x001c95b0
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return kHudItemAutocatcher.
     * @ghidraAddress 0x001c95a8
     */
    virtual int Type();
};
