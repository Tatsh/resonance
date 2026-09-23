#pragma once

#include <vector>

#include "game/powerupcollectioni.h"

class LocalPlayer;
class Powerup;

/**
 * Store of one powerup of every configured kind, with a count for each.
 *
 * `17PowerupCollection` in the RTTI descriptor at `0x008ef460`, with PowerupCollectionI as its one
 * base at offset 0. Its table is at `0x007e48a8` and has ten entries with a zero terminator at
 * index 10, the same length as the base table, so the class adds no virtual. It overrides the
 * destructor and all six of the base's own slots.
 *
 * The object is 0x2c bytes. The base occupies the first 0x14 including the inherited vptr at
 * `+0x10`, and the four members below follow it.
 *
 * LocalPlayer's constructor allocates one at `0x0011e0ec` for one of its modes and stores the
 * pointer at its own `+0xa4`, then hands the same pointer to the JamPowerupPlacer it allocates
 * next. Its other modes allocate a SinglePowerupCollection instead.
 *
 * The class manages every powerup it stores. The destructor deletes each one through the Powerup
 * table and then releases the vector.
 *
 * Every method name is inferred from the bodies. No method name survives anywhere in the image.
 */
class PowerupCollection : public PowerupCollectionI {
public:
    /**
     * One configured kind, with the number of them stored.
     *
     * `Entry` is a placeholder for the name, which no descriptor, allocation tag, or literal in
     * the image supplies. The record is eight bytes and the vector steps over it in eights.
     *
     * Both members are public, because the only code that reads either is a member of
     * PowerupCollection and the record has no behaviour of its own.
     */
    struct Entry {
        Powerup *mPowerup; /*!< The powerup, which the collection deletes. +0x00 */
        int mCount;        /*!< How many are stored, from 0 to 9. +0x04 */
    };

    /**
     * Build one entry for every kind the configuration lists.
     *
     * The kinds come from configuration code 0x389 through QueryConfigVector(), and each one is
     * turned into a powerup by Powerup::CreateForType(). Every entry starts with a count of zero,
     * unless bUnlimited is set, in which case each entry starts with one and the selection starts
     * at the first entry.
     *
     * @param pOwner The player whose collection this is.
     * @param bUnlimited Non-zero to start every entry stocked and to stop Deploy() from spending.
     * @ghidraAddress 0x001cad70
     */
    PowerupCollection(LocalPlayer *pOwner, int bUnlimited);

    /**
     * Delete every stored powerup.
     *
     * @ghidraAddress 0x001cb090
     */
    virtual ~PowerupCollection();

    /**
     * Increase the count of the entry whose kind matches, up to nine.
     *
     * An entry already storing nine is ignored, and a kind with no entry is ignored. A successful
     * increase sends a PowerupCountMsg. With nothing selected, SelectRelative(1) follows.
     *
     * @param nType The kind.
     * @ghidraAddress 0x001cb230
     */
    virtual void AddPowerup(int nType);

    /**
     * Move the selection by nDelta, wrapping, and stop at the first entry with a count above zero.
     *
     * A delta of zero returns with no change and no message. The search tries every entry once
     * and selects -1 when all of them are empty. Either outcome sends a ChoosePowerupMsg.
     *
     * @param nDelta The step to apply to the selected index.
     * @ghidraAddress 0x001cb320
     */
    virtual void SelectRelative(int nDelta);

    /**
     * Select one entry by index, without checking its count.
     *
     * @param nIndex The entry, or -1 for none.
     * @ghidraAddress 0x001cb450
     */
    virtual void Select(int nIndex);

    /**
     * Deploy the selected powerup and spend one of them.
     *
     * The selected entry is read with no test against -1, so a caller selects first. A powerup
     * that reports failure stops the whole body. With mUnlimited set the count is preserved and no
     * message follows. Otherwise the count drops by one, a PowerupCountMsg follows, and an entry
     * that reaches zero triggers SelectRelative(1).
     *
     * @param nTrack Forwarded to Powerup::Deploy(). See PowerupCollectionI::Deploy().
     * @param nBar Forwarded to Powerup::Deploy(). See PowerupCollectionI::Deploy().
     * @ghidraAddress 0x001cb500
     */
    virtual void Deploy(int nTrack, int nBar);

    /**
     * Report whether an entry is selected.
     *
     * @return Non-zero unless the selected index is -1.
     * @ghidraAddress 0x001cc9a0
     */
    virtual int HasSelection();

    /**
     * Send one PowerupCountMsg for every non-empty entry, then one ChoosePowerupMsg.
     *
     * @ghidraAddress 0x001cb620
     */
    virtual void AnnounceState();

private:
    std::vector<Entry> mEntries; // +0x14
    // The selected entry, or -1 for none.
    int mSelected;       // +0x20
    LocalPlayer *mOwner; // +0x24
    // Set from the constructor's second argument. It stocks every entry at construction and stops
    // Deploy() from spending, so a deployment costs nothing while it is set.
    int mUnlimited; // +0x28
};
