#pragma once

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

    /** @ghidraAddress 0x0012ad58 */
    virtual void Slot5();

    /** @ghidraAddress 0x00128d08 */
    virtual void Slot6();

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
