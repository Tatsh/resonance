#pragma once

#include <vector>

#include "game/playmap.h"

/**
 * Traversal that wraps from the end of the sequence back to the start.
 *
 * `PlayMapRing` in the RTTI descriptor at `0x00901dd0`, with `PlayMap` as its only base at offset
 * 0. Its table runs the same nineteen entries as the base, adding none, and it fills the two the
 * base leaves pure, so the class is concrete. Beyond slots 1, 5, 6, and 8 every entry is the base
 * implementation, which is the narrowest override set of the three subclasses.
 *
 * No member is recovered, so none is declared, and the class adds no data to the 0x3c bytes of the
 * base. The constructor at `0x0012daa0` is implicit for the same reason the destructor below is.
 * It runs the PlayMap constructor and installs the table at `0x007d1598`, and no routine in the
 * image calls it.
 *
 * The destructor at `0x0012e178` is declared nowhere here. Its body restores the base table
 * pointer, tears down the four base vectors inline, and releases through the base allocation tag
 * `PlayMap` rather than a tag of its own, which is exactly what an implicit destructor for this
 * class compiles to. An empty destructor a programmer wrote would compile to the same bytes, so
 * the two cannot be told apart, and the convention of this tree resolves that by declaring
 * neither. The routine still occupies slot 1 of this class's own table, because the compiler
 * emits an implicit destructor per class.
 */
class PlayMapRing : public PlayMap {
public:
    /**
     * Wraps the position into one turn of the ring.
     *
     * The body returns `(nValue + mBarCount) % mSteps.back()`, so mSteps.back() is the length of
     * one turn and mBarCount is the offset the ring starts at.
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

    /**
     * Scales one turn of the ring by a repeat count.
     *
     * The repeat count is configuration code 0x385, read through QueryConfigValue().
     *
     * @return One turn scaled by the repeat count.
     * @ghidraAddress 0x0012e430
     */
    virtual int Slot8();
};
