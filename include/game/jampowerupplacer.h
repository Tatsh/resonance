#pragma once

#include "app/msgsource.h"
#include "game/localplayer.h"
#include "game/powerupplacer.h"

class PowerupCollectionI;

/**
 * Powerup placer a LocalPlayer owns.
 *
 * `16JamPowerupPlacer` in the RTTI descriptor at `0x008eff40`, with PowerupPlacer as its one base
 * at offset 0, which in turn derives from MsgSource. Its table is at `0x007e4b80` and has nine
 * entries with a zero terminator at index 9.
 *
 * Of those nine it overrides only the destructor at slot 1 and slot 8. Slots 2 and 3 retain the
 * MsgSource pair and slots 4 through 7 retain the empty PowerupPlacer defaults, at the same four
 * addresses the base table records rather than at copies of its own.
 *
 * The object is 0x1c bytes. The base occupies the first 0x14 including the inherited vptr at
 * `+0x10`, and the two members below follow it. LocalPlayer allocates one in its constructor at
 * `0x0011e118` and stores the pointer at its own `+0xa8`.
 */
class JamPowerupPlacer : public PowerupPlacer {
public:
    /**
     * @param pOwner The player that owns this placer.
     * @param pCollection The powerup collection the owner holds at its own `+0xa4`.
     * @ghidraAddress 0x001cdf88
     */
    JamPowerupPlacer(LocalPlayer *pOwner, PowerupCollectionI *pCollection);

    /**
     * @ghidraAddress 0x001cdc58
     */
    virtual ~JamPowerupPlacer();

    /**
     * Deploy the owner's selected powerup on the current bar. Slot 8, overriding the base.
     *
     * The bar is the song position divided by the 1920 ticks of a bar, and the track is the
     * owner's Player slot 4. Both go to PowerupCollectionI::Deploy() on mCollection.
     *
     * @ghidraAddress 0x001cdfe0
     */
    virtual void OnUnknownSlot8();

private:
    LocalPlayer *mOwner;             // +0x14
    PowerupCollectionI *mCollection; // +0x18
};
