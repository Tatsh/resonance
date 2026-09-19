#pragma once

#include <vector>

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

    /**
     * Wraps the position into one turn of the ring.
     *
     * The body returns `(nValue + mUnknown00) % mSteps.back()`, so mSteps.back() is the length of
     * one turn and mUnknown00 is the offset the ring starts at.
     *
     * @param nValue The position to wrap.
     * @return The wrapped position.
     * @ghidraAddress 0x0012e408
     */
    virtual int Slot5(int nValue);

    /**
     * Collects every position of one span into mUnknown2c.
     *
     * The body clears mUnknown2c, then walks a value from nStart upward in steps of mSteps.back()
     * while it remains below nEnd, appending each value that is not below nMin.
     *
     * @param nStart The first position.
     * @param nMin The lowest position to collect.
     * @param nEnd The position to stop below.
     * @return mUnknown2c.
     * @ghidraAddress 0x0012dad8
     */
    virtual std::vector<int> &Slot6(int nStart, int nMin, int nEnd);

    /** @ghidraAddress 0x0012e430 */
    virtual int Slot8();
};
