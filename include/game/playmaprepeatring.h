#pragma once

#include <vector>

#include "game/playmap.h"

/**
 * Traversal that wraps and repeats a section of the sequence.
 *
 * `PlayMapRepeatRing` in the RTTI descriptor at `0x00901dc0`, with `PlayMap` as its only base at
 * offset 0. It overrides slots 1, 3 through 6, and 15 through 19. It is the only subclass to
 * replace slot 3, which in the base merely stores its argument, and the only one besides
 * `PlayMapLinear` to supply slot 19.
 *
 * Slot 20 is pure in the base and unimplemented here, so this class is abstract as well unless a
 * further subclass supplies it. No member is recovered, so none is declared.
 */
class PlayMapRepeatRing : public PlayMap {
public:
    /** @ghidraAddress 0x0012d1b8 */
    virtual ~PlayMapRepeatRing();

    /** @ghidraAddress 0x0012bb60 */
    virtual void Slot3(int nValue);

    /** @ghidraAddress 0x0012ba88 */
    virtual void Slot4();

    /**
     * Maps the position into one turn of the repeating ring.
     *
     * The body is not reconstructed.
     *
     * @param nValue The position to map.
     * @return The mapped position.
     * @ghidraAddress 0x0012d500
     */
    virtual int Slot5(int nValue);

    /**
     * Collects every position of one span into mUnknown2c.
     *
     * The body is not reconstructed.
     *
     * @param nStart The first position.
     * @param nMin The lowest position to collect.
     * @param nEnd The position to stop below.
     * @return mUnknown2c.
     * @ghidraAddress 0x0012be68
     */
    virtual std::vector<int> &Slot6(int nStart, int nMin, int nEnd);

    /** @ghidraAddress 0x0012bc48 */
    virtual void Slot15();

    /** @ghidraAddress 0x0012bd00 */
    virtual int Slot16();

    /** @ghidraAddress 0x0012bdd8 */
    virtual int Slot17();

    /** @ghidraAddress 0x0012d5d0 */
    virtual int Slot18();

    /** @ghidraAddress 0x0012d4b0 */
    virtual void Slot19();
};
