#pragma once

#include "app/hudfreq.h"
#include "app/hudscore.h"

class Player;

/**
 * One player's badge on the head-up display, a score readout over a FreQ icon.
 *
 * The class is not polymorphic and emits no RTTI. Overlay allocates one per world player with the
 * untagged scalar allocator (0x2c bytes, the size its constructor requests at `0x0041cfd4`) and
 * stores the pointers in its vector at `+0x14`. Overlay's destructor releases them with the scalar
 * free and runs no destructor, and the class therefore declares none. No descriptor, tag, or file
 * path identifies the class, and its name is inferred from its two parts.
 *
 * Overlay's constructor inlines the constructor. The members are public because code outside the
 * class accesses them directly. Overlay looks a badge up by mPlayer and marks the leader through
 * mFreq, and HudScorePulse reads the position of mScore's mesh.
 */
class HudBadge {
public:
    /**
     * Build the badge for one player.
     *
     * @param pPlayer The player the badge shows.
     * @param nIndex The badge number, counted over the world's players from 0.
     * @ghidraAddress 0x0042ac58
     */
    HudBadge(Player *pPlayer, int nIndex);

    /**
     * Advance the icon to the song position and redraw a settled score.
     *
     * @param flFrame The song position, in MIDI ticks.
     * @param flTime The time HudScore::Update() compares against.
     * @ghidraAddress 0x0041c0f8
     */
    void SetFrame(float flFrame, float flTime);

    HudScore mScore; /*!< The score readout. +0x00 */
    HudFreq mFreq;   /*!< The FreQ icon. +0x10 */
    Player *mPlayer; /*!< The player the badge shows. +0x28 */
};
