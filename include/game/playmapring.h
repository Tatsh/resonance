#pragma once

#include "game/playmap.h"

/**
 * Traversal that wraps from the end of the sequence back to the start.
 *
 * `PlayMapRing` in the RTTI descriptor at `0x00901dd0`, with `PlayMap` as its only base at offset
 * 0. It overrides the narrowest set of the three subclasses, slots 1, 5, 6, and 8, so everything
 * else is the base implementation. Slots 19 and 20 are pure in the base and unimplemented here,
 * which means this class is abstract as well, or that a fourth subclass supplies them.
 *
 * No member is recovered, so none is declared.
 */
class PlayMapRing : public PlayMap {
public:
    /** @ghidraAddress 0x0012e178 */
    virtual ~PlayMapRing();

    /** @ghidraAddress 0x0012e408 */
    virtual void Slot5();

    /** @ghidraAddress 0x0012dad8 */
    virtual void Slot6();

    /** @ghidraAddress 0x0012e430 */
    virtual int Slot8();
};
