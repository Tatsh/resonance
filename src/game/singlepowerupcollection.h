#pragma once

#include "game/powerupcollectioni.h"

class LocalPlayer;
class Powerup;

/**
 * Store of one powerup at a time, which a new one replaces.
 *
 * Its RTTI descriptor is at `0x008efed0`. It has PowerupCollectionI as its one base at offset 0.
 * Its table is at `0x007e4850` and has ten entries with a zero terminator at index 10, the same
 * length as the base table. The class adds no virtual.
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
     * @ghidraAddress NTSC-U/C: 0x001cb760
     * @ghidraAddress PAL: 0x001d1618
     */
    explicit SinglePowerupCollection(LocalPlayer *pOwner);

    /**
     * Delete the stored powerup.
     *
     * @ghidraAddress NTSC-U/C: 0x001cb7b0
     * @ghidraAddress PAL: 0x001d1668
     */
    virtual ~SinglePowerupCollection();

    /**
     * Replace the stored powerup with one of the requested kind.
     *
     * The powerup already stored is deleted first, whatever its kind, so a store of one never
     * queues. A ChoosePowerupMsg follows with the new kind.
     *
     * @param type The kind.
     * @ghidraAddress NTSC-U/C: 0x001cb890
     * @ghidraAddress PAL: 0x001d1748
     */
    virtual void Add(PowerupType type);

    /**
     * Do nothing. A store of one has no other entry to move to.
     *
     * An empty override of its own at table slot 5, apart from the base's empty default.
     *
     * @param nDelta Not read.
     * @ghidraAddress NTSC-U/C: 0x001cc9f0
     * @ghidraAddress PAL: 0x001d28a8
     */
    virtual void SelectRelative(int nDelta);

    /**
     * Do nothing. A store of one has no other entry to select.
     *
     * An empty override of its own at table slot 6, apart from the base's empty default.
     *
     * @param nIndex Not read.
     * @ghidraAddress NTSC-U/C: 0x001cc9f8
     * @ghidraAddress PAL: 0x001d28b0
     */
    virtual void Select(int nIndex);

    /**
     * Deploy the stored powerup and discard it.
     *
     * An empty store returns with no message, and a powerup that reports failure keeps its place.
     * A success sends a ChoosePowerupMsg with a kind of -1, then deletes the powerup and clears
     * both members.
     *
     * @param nTrack Forwarded to Powerup::Deploy(). See PowerupCollectionI::Deploy().
     * @param nBar Forwarded to Powerup::Deploy(). See PowerupCollectionI::Deploy().
     * @ghidraAddress NTSC-U/C: 0x001cb948
     * @ghidraAddress PAL: 0x001d1800
     */
    virtual void Deploy(int nTrack, int nBar);

    /**
     * Report whether a powerup is stored.
     *
     * @return Non-zero unless the stored kind is -1.
     * @ghidraAddress NTSC-U/C: 0x001cca00
     * @ghidraAddress PAL: 0x001d28b8
     */
    virtual int HasSelection() const;

    /**
     * Send the stored kind again, for a listener that has just registered.
     *
     * An empty store sends nothing. The test goes through the virtual HasSelection() rather than
     * reading the member.
     *
     * @ghidraAddress NTSC-U/C: 0x001cba20
     * @ghidraAddress PAL: 0x001d18d8
     */
    virtual void SendState() const;

private:
    // The stored kind, or -1 for an empty store.
    int mType;           // +0x14
    Powerup *mPowerup;   // +0x18
    LocalPlayer *mOwner; // +0x1c
};
