#pragma once

#include "app/msgsource.h"

/**
 * Base of the objects that place a powerup for a player.
 *
 * `13PowerupPlacer` in the RTTI descriptor at `0x008efc10`, with MsgSource as its one base at
 * offset 0. Its own table is at `0x007e4da0` and has nine entries with a zero terminator at index
 * 9. Slots 2 and 3 retain the MsgSource pair at `0x0054a270` and `0x0054a2f0`, and the five slots
 * after them are this class's own.
 *
 * The class adds no data member. Its constructor at `0x001cd958` stores the table pointer at
 * `+0x10` and does nothing else, which places the object at the same 0x14 bytes MsgSource occupies.
 *
 * Two subclasses are attested, JamPowerupPlacer and a `17GamePowerupPlacer` whose mangled name sits
 * beside this one at `0x007e4ca0`. The second has no header yet.
 *
 * Slots 4 through 7 are each a two-instruction `jr ra` stub, so the defaults do nothing. Both
 * tables record the same four addresses rather than a copy each, which is what establishes that a
 * subclass inherits them rather than re-emitting an empty body of its own. LocalPlayer::Slot11
 * dispatches slot 4 and LocalPlayer::Slot12 dispatches slot 5, and against a JamPowerupPlacer both
 * therefore do nothing.
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
     * Unrecovered. Slot 4, and an empty default.
     *
     * The parameter list is unrecovered. LocalPlayer::Slot11 is its one recovered caller.
     *
     * @ghidraAddress 0x001cd990
     */
    virtual void OnUnknownSlot4();

    /**
     * Unrecovered. Slot 5, and an empty default.
     *
     * The parameter list is unrecovered. LocalPlayer::Slot12 is its one recovered caller.
     *
     * @ghidraAddress 0x001cd998
     */
    virtual void OnUnknownSlot5();

    /**
     * Unrecovered. Slot 6, and an empty default.
     *
     * @ghidraAddress 0x001cd9a0
     */
    virtual void OnUnknownSlot6();

    /**
     * Unrecovered. Slot 7, and an empty default.
     *
     * @ghidraAddress 0x001cd9a8
     */
    virtual void OnUnknownSlot7();

    /**
     * Unrecovered. Slot 8, and the one slot of the five with a body.
     *
     * JamPowerupPlacer overrides it at `0x001cdfe0`.
     *
     * @ghidraAddress 0x001ce1b0
     */
    virtual void OnUnknownSlot8();
};
