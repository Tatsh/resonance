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

    /**
     * Reports a step index for the mapped position.
     *
     * Declared void and nil-ary until the base's own signature was recovered, which made this a
     * new virtual rather than an override: a differing parameter list compiles, extends the table,
     * and silently detaches. The body proves both halves of the real signature, reading a1 into a
     * saved register as its first act and returning a shifted difference.
     *
     * The body is not written. It passes its argument through the helper at `0x00129150` twice,
     * upper-bounds a vector of its own at `+0x48`, and then combines that result with two further
     * members, none of which is recovered.
     *
     * @param nValue The position to map.
     * @return The step index.
     * @ghidraAddress 0x0012af30
     */
    virtual int Slot12(int nValue);

    /**
     * Reports the same index against a different member.
     *
     * Declared void and nil-ary for the same reason as slot 12, and detached in the same way. The
     * body performs the same argument-through-helper and upper-bound sequence and then reads the
     * member at `+0x54` rather than the pair slot 12 reads.
     *
     * The body is not written, for the same reason.
     *
     * @param nValue The position to map.
     * @return The step index.
     * @ghidraAddress 0x0012afb8
     */
    virtual int Slot13(int nValue);

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
