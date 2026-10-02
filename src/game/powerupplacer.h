#pragma once

#include "app/msgsource.h"

/**
 * Base of the objects that place a powerup for a player.
 *
 * Its RTTI descriptor is at `0x008efc10`. It has MsgSource as its one base at offset 0. Its own
 * table is at `0x007e4da0` and has nine entries with a zero terminator at index 9. Slots 2 and 3
 * retain the MsgSource pair at `0x0054a270` and `0x0054a2f0`, and the five slots after them are
 * this class's own.
 *
 * The class adds no data member. Its constructor at `0x001cd958` stores the table pointer at
 * `+0x10` and does nothing else, which places the object at the same 0x14 bytes MsgSource occupies.
 *
 * Two subclasses are attested, JamPowerupPlacer and GamePowerupPlacer, whose type name sits beside
 * this one at `0x007e4ca0`. The second has no header yet.
 *
 * Slots 4 through 7 are each a two-instruction `jr ra` stub, so the defaults do nothing. Both
 * tables record the same four addresses rather than a copy each, which is what establishes that a
 * subclass inherits them rather than re-emitting an empty body of its own. LocalPlayer dispatches
 * Activate() and Deactivate(), and against a JamPowerupPlacer both therefore do nothing.
 */
class PowerupPlacer : public MsgSource {
public:
    /**
     * @ghidraAddress 0x001cd958
     */
    PowerupPlacer();

    /**
     * @ghidraAddress 0x001ce0b0
     */
    virtual ~PowerupPlacer();

    /**
     * Start placing as the owning player starts. Slot 4, and an empty default.
     *
     * The owning LocalPlayer dispatches it while it starts, before it announces its track.
     * GamePowerupPlacer's override starts its tick task.
     *
     * @ghidraAddress 0x001cd990
     */
    virtual void Activate();

    /**
     * Stop placing. Slot 5, and an empty default.
     *
     * GamePowerupPlacer's override stops its tick task.
     *
     * @ghidraAddress 0x001cd998
     */
    virtual void Deactivate();

    /**
     * Move the placement cursor. Slot 6, and an empty default.
     *
     * The parameter comes from GamePowerupPlacer's override at `0x001cccb0`, which returns at once
     * for an argument of zero and otherwise negates it and adds it to a bar cursor. An empty
     * default reveals no parameter list of its own, so the arity rests on that one override.
     *
     * @param nStep The step, whose meaning beyond a signed increment is unrecovered.
     * @ghidraAddress 0x001cd9a0
     */
    virtual void MoveCursor(int nStep);

    /**
     * Announce where the placement cursor rests. Slot 7, and an empty default.
     *
     * The owning LocalPlayer dispatches it after a track selection.
     *
     * @ghidraAddress 0x001cd9a8
     */
    virtual void AnnounceCursor();

    /**
     * Deploy the owner's selected powerup. Slot 8, and the one slot of the five with a body.
     *
     * The owning LocalPlayer dispatches it on a ButtonPowMsg. JamPowerupPlacer overrides it at
     * `0x001cdfe0`.
     *
     * @ghidraAddress 0x001ce1b0
     */
    virtual void DeployPowerup();
};
