#pragma once

#include "game/powerupcollectioni.h"

class LocalPlayer;
class Powerup;

/**
 * Store of one powerup at a time, which a new one replaces.
 *
 * `23SinglePowerupCollection` in the RTTI descriptor at `0x008efed0`, with PowerupCollectionI as
 * its one base at offset 0. Its table is at `0x007e4850` and has ten entries with a zero
 * terminator at index 10, the same length as the base table, so the class adds no virtual.
 *
 * It overrides the destructor and four of the base's six slots. Slots 5 and 6, SelectRelative()
 * and Select(), point at two-instruction bodies of their own at `0x001cc9f0` and `0x001cc9f8`, and
 * this toolchain re-emits an inline empty body into every translation unit that needs one, so a
 * unique address for an empty body is no evidence of an override. Both are recorded as inherited
 * and neither is declared. Selecting has no meaning for a store of one.
 *
 * The object is 0x20 bytes. The base occupies the first 0x14 including the inherited vptr at
 * `+0x10`, and the three members below follow it.
 *
 * LocalPlayer's constructor allocates one at `0x0011e170` for three of its modes and stores the
 * pointer at its own `+0xa4`. Its remaining mode allocates a PowerupCollection instead.
 *
 * Every method name is inferred from the bodies. No method name survives anywhere in the image.
 */
class SinglePowerupCollection : public PowerupCollectionI {
public:
    /**
     * Start with no powerup stored.
     *
     * @param pOwner The player whose collection this is.
     * @ghidraAddress 0x001cb760
     */
    explicit SinglePowerupCollection(LocalPlayer *pOwner);

    /**
     * Delete the stored powerup.
     *
     * @ghidraAddress 0x001cb7b0
     */
    virtual ~SinglePowerupCollection();

    /**
     * Replace the stored powerup with one of the requested kind.
     *
     * The powerup already stored is deleted first, whatever its kind, so a store of one never
     * queues. A ChoosePowerupMsg follows with the new kind.
     *
     * @param nType The kind.
     * @ghidraAddress 0x001cb890
     */
    virtual void AddPowerup(int nType);

    /**
     * Deploy the stored powerup and discard it.
     *
     * An empty store returns with no message, and a powerup that reports failure keeps its place.
     * A success sends a ChoosePowerupMsg with a kind of -1, then deletes the powerup and clears
     * both members.
     *
     * @param nPlayerValue Ignored. See PowerupCollectionI::Deploy().
     * @param nBar Ignored. See PowerupCollectionI::Deploy().
     * @ghidraAddress 0x001cb948
     */
    virtual void Deploy(int nPlayerValue, int nBar);

    /**
     * Report whether a powerup is stored.
     *
     * @return Non-zero unless the stored kind is -1.
     * @ghidraAddress 0x001cca00
     */
    virtual int HasSelection();

    /**
     * Send the stored kind again, for a listener that has just registered.
     *
     * An empty store sends nothing. The test goes through the virtual HasSelection() rather than
     * reading the member.
     *
     * @ghidraAddress 0x001cba20
     */
    virtual void AnnounceState();

private:
    // The stored kind, or -1 for an empty store.
    int mType;           // +0x14
    Powerup *mPowerup;   // +0x18
    LocalPlayer *mOwner; // +0x1c
};
