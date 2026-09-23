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
 * The object is 0x74 bytes, which the tagged allocation in the LevelBuilder constructor at
 * `0x001ea8f0` measures. Its own members follow the base at `+0x3c`. The first four are recovered
 * from the helper at `0x00129150`, which is the only routine that reads all four and the reason
 * three of this class's slots could not be written before it was decoded. That helper advances a
 * window forward over the source span and records how far it has advanced. The constructor at
 * `0x00127a80` and the destructor add the last two.
 *
 * The earlier note that the members start at `+0x2c` was wrong. The base occupies 0x3c bytes, and
 * `+0x2c` is the base's fourth vector rather than anything of this class.
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

    /**
     * Carries a position from its step to the partner step the table for one set records.
     *
     * The body is not written. It finds the position's step through PlayMap::FindStepIndex(),
     * scans the eight-byte records of mUnknown68[nSet] for one whose first word is that step, and
     * returns the position moved by the distance between the two steps. Without such a record the
     * position comes back unchanged.
     *
     * @param nValue The position to map.
     * @param nSet The index into mUnknown68.
     * @return The mapped position.
     * @ghidraAddress 0x0012ae88
     */
    virtual int Slot7(int nValue, int nSet);

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

    /**
     * @param nValue The position.
     * @return 1 when the second word of the `+0x3c` record for the last step at or before the
     *         position is 10000, and 0 otherwise.
     * @ghidraAddress 0x0012b028
     */
    virtual int Slot14(int nValue);

    /** @ghidraAddress 0x00128ed8 */
    virtual int Slot16(int nBar);

    /** @ghidraAddress 0x00129038 */
    virtual int Slot17(int nBar);

    /** @ghidraAddress 0x0012b0a8 */
    virtual int Slot18(int nBar);

    /** @ghidraAddress 0x0012aa18 */
    virtual void Slot19();

    /**
     * Slot 20. Calls slot 8 through the table, then walks the span at `+0x48`.
     *
     * @ghidraAddress 0x00128c38
     */
    virtual void Slot20();

protected:
    /**
     * One element of the window, a value from the source span paired with a flag.
     *
     * Eight bytes. The helper builds each one from a word of mUnknown58 and sets the flag to 1,
     * and nothing recovered so far sets it to anything else or reads it back, so the flag's
     * purpose is unrecovered and only its initial value is known.
     */
    struct Entry {
        int mValue;
        int mFlag;
    };

    /**
     * Advance the window until slot 8 passes the limit, then drop what it has passed.
     *
     * Non-virtual, and every one of slots 5, 12, and 13 calls it twice with its own argument before
     * doing anything else. The growth half appends one element to mUnknown48 from slot 8's result
     * and one Entry to mUnknown3c per element of mUnknown58, and the test is at the top of the loop
     * so the body can run zero times. Slot 8 is dispatched through the table rather than called
     * directly, so a further subclass would change what the loop grows towards.
     *
     * The trim half drops as many leading elements from mUnknown3c and mUnknown48 as mUnknown58
     * holds, and adds that count to mUnknown54. Its guard compares a byte offset against an
     * element count, which both the disassembly and the decompiler agree on, so the trim fires
     * only once mUnknown3c is more than eight times the length of mUnknown58.
     *
     * @param nLimit The value slot 8 must exceed for the growth to stop.
     * @ghidraAddress 0x00129150
     */
    void GrowPastLimit(int nLimit);

    // Declared in recovered offset order. The helper above reads the first four.
    std::vector<Entry> mUnknown3c; // +0x3c
    std::vector<int> mUnknown48;   // +0x48
    // Running count of elements the trim has dropped from the front of the two vectors above.
    int mUnknown54; // +0x54
    // The source the window is built from. Its element is eight bytes and only the first word is
    // read, so the second word's purpose is unrecovered.
    std::vector<Entry> mUnknown58; // +0x58
    // Cleared by the constructor at 0x00127a80. No reader is recovered.
    int mUnknown64; // +0x64
    // The constructor reserves eight elements of twelve bytes, and the destructor frees each
    // element's own eight-byte-stride buffer before freeing this one, so every element is a
    // vector of Entry.
    std::vector<std::vector<Entry> > mUnknown68; // +0x68
};
