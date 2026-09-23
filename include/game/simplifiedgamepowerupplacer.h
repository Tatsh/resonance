#pragma once

#include "game/powerupplacer.h"

class LocalPlayer;
class PowerupCollectionI;

/**
 * Powerup placer a LocalPlayer owns outside jam.
 *
 * `27SimplifiedGamePowerupPlacer` in the RTTI descriptor, with PowerupPlacer as its one base. Its
 * table is at `0x007e4bd0` and has nine entries. Like JamPowerupPlacer it overrides only the
 * destructor and slot 8, and retains the base defaults for slots 4 through 7. Its unit spans
 * `0x001cdb20` through `0x001cdf84`.
 *
 * The object is 0x1c bytes. LocalPlayer's constructor at `0x0011e198` allocates one in game modes 1
 * through 3 and stores the pointer at its own `+0xa8`, with the SinglePowerupCollection it built
 * first as the collection.
 */
class SimplifiedGamePowerupPlacer : public PowerupPlacer {
public:
    /**
     * @param pOwner The player that owns this placer.
     * @param pCollection The owner's powerup collection.
     * @ghidraAddress 0x001cde60
     */
    SimplifiedGamePowerupPlacer(LocalPlayer *pOwner, PowerupCollectionI *pCollection);

    /**
     * The body is the inlined base destructor.
     *
     * @ghidraAddress 0x001cdb20
     */
    virtual ~SimplifiedGamePowerupPlacer();

    /**
     * Deploy the selected powerup on the owner's track at the current bar.
     *
     * Does nothing from the level's last bar on, which PlayMap::Slot9() reports.
     *
     * @ghidraAddress 0x001cdeb8
     */
    virtual void OnUnknownSlot8();

private:
    LocalPlayer *mOwner;             // +0x14
    PowerupCollectionI *mCollection; // +0x18
};
