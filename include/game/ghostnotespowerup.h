#pragma once

#include "game/powerup.h"

/**
 * Powerup that lights the player's track guides.
 *
 * `17GhostNotesPowerup` in the RTTI descriptor at `0x008f0900`, with Powerup as its one base. The
 * factory builds it for kHudItemGuides with a four-byte allocation and the table at `0x007e4040`.
 * The destructor at `0x001ca030` is implicitly declared.
 */
class GhostNotesPowerup : public Powerup {
public:
    /**
     * Deliver a ToggleGhostMsg that switches the guides on straight to the player's sink.
     *
     * @param nTrack Not read.
     * @param nBar Not read.
     * @param pPlayer The deploying player.
     * @param nUnused Not read.
     * @return 1 always.
     * @ghidraAddress 0x001ca0e8
     */
    virtual int Deploy(int nTrack, int nBar, Player *pPlayer, int nUnused);

    /**
     * @return kHudItemGuides.
     * @ghidraAddress 0x001ca0e0
     */
    virtual int Type();
};
