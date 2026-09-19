#pragma once

#include <vector>

/**
 * Traversal order over one playable sequence.
 *
 * `PlayMap` in the RTTI descriptor at `0x0086f5b0`, a leaf with no base list. The class is
 * abstract: no data reference to its type function exists anywhere in the image, so no vtable
 * carries it at slot 0 and the class is never instantiated. Three subclasses derive from it,
 * `PlayMapLinear`, `PlayMapRing`, and `PlayMapRepeatRing`, which is what the two pure slots below
 * are for.
 *
 * The table runs to nineteen entries, slots 0 through 18, and terminates on the all-zero entry
 * after them. This class implements every slot except 5 and 6, which are the only two pointing at
 * the shared pure-virtual stub. Most of what it does implement is inert, one slot being a bare
 * return and four returning zero, so the base reads as an interface with defaults rather than as
 * shared behaviour.
 *
 * The subclasses fill those two and then extend the table rather than overriding past its end:
 * `PlayMapRing` adds nothing and keeps nineteen entries, `PlayMapRepeatRing` adds one for twenty,
 * and `PlayMapLinear` adds two for twenty-one. None of the three retains a slot pointing at the
 * pure-virtual stub, so all three are concrete, which also settles a question raised while they
 * were recovered: no fourth subclass is missing, and the harvest records exactly these three.
 *
 * The object is 0x3c bytes with the vptr at `+0x38`. Its four vectors are recovered from the
 * destructor at `0x00127158`, which tears them down at `+0x2c`, `+0x1c`, `+0x10`, and `+0x04` and
 * so fixes their declaration order as the reverse. Element widths come from the shift each
 * teardown uses to divide the byte span.
 *
 * Every slot below whose verb is unrecovered keeps its table index as its title. The index is part
 * of the class layout, so a slot is declared whether or not its purpose is known, and the comment
 * records the behaviour that was recovered instead.
 */
class PlayMap {
public:
    /**
     * One entry of the third vector.
     *
     * Eight bytes, of which the destructor releases the second word, so that word owns its
     * allocation. Neither field's purpose is recovered.
     */
    struct Entry {
        int mUnknown00;   // +0x00
        void *mUnknown04; // +0x04 released by ~PlayMap
    };

    /** @ghidraAddress 0x00127158 */
    virtual ~PlayMap();

    /**
     * Slot 2. Shifts a position by the last element of mSteps.
     *
     * @ghidraAddress 0x001268b0
     */
    virtual void Slot2(int nPosition, int nArg);

    /**
     * Slot 3. Stores its argument in mUnknown00 and does nothing else.
     *
     * @ghidraAddress 0x00127488
     */
    virtual void Slot3(int nValue);

    /**
     * Slot 4. Empty in this class.
     *
     * @ghidraAddress 0x00127398
     */
    virtual void Slot4();

    /** Slot 5. Pure here; every subclass overrides it. */
    virtual void Slot5() = 0;

    /** Slot 6. Pure here; every subclass overrides it. */
    virtual void Slot6() = 0;

    /**
     * Slot 7. Returns its argument unchanged.
     *
     * @ghidraAddress 0x001273b8
     */
    virtual int Slot7(int nValue);

    /**
     * Slot 8. Returns the last element of mSteps.
     *
     * @ghidraAddress 0x001273c0
     */
    virtual int Slot8();

    /**
     * Slot 9. Forwards to slot 8 through the table rather than calling it directly.
     *
     * @ghidraAddress 0x001273d0
     */
    virtual int Slot9();

    /**
     * Slot 10. Returns the number of elements in mSteps.
     *
     * @ghidraAddress 0x00127440
     */
    virtual int Slot10();

    /**
     * Slot 11. Returns its argument unchanged.
     *
     * @ghidraAddress 0x00127458
     */
    virtual int Slot11(int nValue);

    /** @ghidraAddress 0x001276a0 */
    virtual void Slot12();

    /** @ghidraAddress 0x00127700 */
    virtual void Slot13();

    /**
     * Slot 14. Returns zero.
     *
     * @ghidraAddress 0x00127460
     */
    virtual int Slot14();

    /**
     * Slot 15. Empty in this class.
     *
     * @ghidraAddress 0x00127468
     */
    virtual void Slot15();

    /**
     * Slot 16. Returns zero.
     *
     * @ghidraAddress 0x00127470
     */
    virtual int Slot16();

    /**
     * Slot 17. Returns zero.
     *
     * @ghidraAddress 0x00127478
     */
    virtual int Slot17();

    /**
     * Slot 18. Returns zero.
     *
     * @ghidraAddress 0x00127480
     */
    virtual int Slot18();

protected:
    // Declared in recovered offset order. Written by Slot3 and read nowhere yet recovered.
    int mUnknown00; // +0x00
    // Slots 8 and 10 read the last element and the count of this one, which is the only vector
    // whose use is recovered. Its element type is not.
    std::vector<int> mSteps;       // +0x04
    std::vector<int> mUnknown10;   // +0x10
    std::vector<Entry> mUnknown1c; // +0x1c
    int mUnknown28;                // +0x28
    std::vector<int> mUnknown2c;   // +0x2c
};
