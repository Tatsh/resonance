#pragma once

#include "app/msgsource.h"

/**
 * Interface of the store a player draws powerups from.
 *
 * `18PowerupCollectionI` in the RTTI descriptor at `0x008f0030`, with MsgSource as its one base at
 * offset 0. The trailing letter belongs to the name the descriptor records rather than being a
 * suffix this tree added. Its table is at `0x007e49d8` and has ten entries with a zero terminator
 * at index 10. Slots 2 and 3 retain the MsgSource pair at `0x0054a270` and `0x0054a2f0`, and the
 * six slots after them are this class's own.
 *
 * The class adds no data member. Its constructor stores the table pointer at `+0x10` and does
 * nothing else, so the object is the same 0x14 bytes MsgSource occupies. The destructor restores
 * the MsgSource table and expands the base destructor, and nothing of its own remains in the body.
 *
 * Every one of the six virtuals below is a two-instruction `jr ra` default, so an unoverridden
 * slot does nothing. The two subclasses are PowerupCollection, which stores one entry per kind
 * with a count, and SinglePowerupCollection, which stores one powerup at a time. LocalPlayer
 * chooses between the two in its constructor at `0x0011e000` and stores the result at its own
 * `+0xa4`.
 *
 * Every method name here is inferred from the two subclass implementations. No method name
 * survives anywhere in the image. The verb of each slot is recovered, so a name describing the
 * behaviour is written rather than a slot number.
 */
class PowerupCollectionI : public MsgSource {
public:
    /**
     * @ghidraAddress 0x001cc7d0
     */
    PowerupCollectionI();

    /**
     * @ghidraAddress 0x001cca90
     */
    virtual ~PowerupCollectionI();

    /**
     * Store one powerup of the requested kind.
     *
     * Slot 4, and an empty default.
     *
     * @param nType The kind, as Powerup::Type() reports it.
     * @ghidraAddress 0x001ccb40
     */
    virtual void AddPowerup(int nType);

    /**
     * Move the selection by one step, skipping an empty entry.
     *
     * Slot 5, and an empty default. PowerupCollection::AddPowerup() calls it with 1 once a first
     * powerup arrives with nothing selected.
     *
     * @param nDelta The step, which the one implementation applies as an addition.
     * @ghidraAddress 0x001ccb48
     */
    virtual void SelectRelative(int nDelta);

    /**
     * Select one entry by index.
     *
     * Slot 6, and an empty default.
     *
     * @param nIndex The entry, or -1 for none.
     * @ghidraAddress 0x001ccb50
     */
    virtual void Select(int nIndex);

    /**
     * Deploy the selected powerup and account for the one used.
     *
     * Slot 7, and an empty default. Neither implementation reads either parameter, and the arity
     * comes from the three call sites instead. PowerupPlacer's three subclasses all dispatch this
     * slot with two arguments, at `0x001ccf30`, `0x001cdeb8`, and `0x001cdfe0`, so the signature
     * cannot be recovered from the implementations alone.
     *
     * @param nPlayerValue Whatever Player::Slot4() reports for the deploying player. Its meaning
     *                     is unrecovered, and the default in Player returns -1.
     * @param nBar The current bar, as the song tick divided by 1920.
     * @ghidraAddress 0x001ccb58
     */
    virtual void Deploy(int nPlayerValue, int nBar);

    /**
     * Report whether an entry is selected.
     *
     * Slot 8, and an empty default. The default leaves the return register untouched, so the value
     * it yields is indeterminate and the base is not meant to be called. The return type comes
     * from the two implementations, which both compute a truth value into the register.
     *
     * @return Non-zero once an entry is selected.
     * @ghidraAddress 0x001ccb60
     */
    virtual int HasSelection();

    /**
     * Send the whole state again, for a listener that has just registered.
     *
     * Slot 9, and an empty default.
     *
     * @ghidraAddress 0x001ccb68
     */
    virtual void AnnounceState();
};
