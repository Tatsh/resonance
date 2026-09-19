#pragma once

#include <vector>

#include "game/playmap.h"

/**
 * Traversal that runs the sequence once from start to end.
 *
 * `PlayMapLinear` in the RTTI descriptor at `0x008f2a20`, with `PlayMap` as its only base at
 * offset 0. It overrides the widest set of the three subclasses, slots 1, 5 through 14, and 16
 * through 19, and supplies slot 20, which no other subclass does.
 *
 * Its own members start at `+0x2c` of the derived object and include a second span at `+0x48` and
 * `+0x4c` that slot 20 walks. Neither is recovered further, so no member is declared here yet.
 */
class PlayMapLinear : public PlayMap {
public:
    /** @ghidraAddress 0x0012a4d8 */
    virtual ~PlayMapLinear();

    /**
     * Maps the position through the linear sequence.
     *
     * The body is not reconstructed. It calls two helpers at `0x00129150` and `0x0012ade8`, indexes
     * three of the object's tables with the result, and finishes with a remainder, so the mapping
     * is table-driven rather than the single wrap PlayMapRing performs.
     *
     * @param nValue The position to map.
     * @return The mapped position.
     * @ghidraAddress 0x0012ad58
     */
    virtual int Slot5(int nValue);

    /**
     * Collects every position of one span into mUnknown2c.
     *
     * @param nStart The first position.
     * @param nMin The lowest position to collect.
     * @param nEnd The position to stop below.
     * @return mUnknown2c.
     * @ghidraAddress 0x00128d08
     */
    virtual std::vector<int> &Slot6(int nStart, int nMin, int nEnd);

    /** @ghidraAddress 0x0012ae88 */
    virtual int Slot7(int nValue);

    /** @ghidraAddress 0x0012ae38 */
    virtual int Slot8();

    /** @ghidraAddress 0x0012aa60 */
    virtual int Slot9();

    /** @ghidraAddress 0x0012aa68 */
    virtual int Slot10();

    /** @ghidraAddress 0x0012aa80 */
    virtual int Slot11(int nValue);

    /** @ghidraAddress 0x0012af30 */
    virtual void Slot12();

    /** @ghidraAddress 0x0012afb8 */
    virtual void Slot13();

    /** @ghidraAddress 0x0012b028 */
    virtual int Slot14();

    /** @ghidraAddress 0x00128ed8 */
    virtual int Slot16();

    /** @ghidraAddress 0x00129038 */
    virtual int Slot17();

    /** @ghidraAddress 0x0012b0a8 */
    virtual int Slot18();

    /** @ghidraAddress 0x0012aa18 */
    virtual void Slot19();

    /**
     * Slot 20. Calls slot 8 through the table, then walks the span at `+0x48`.
     *
     * @ghidraAddress 0x00128c38
     */
    virtual void Slot20();
};
