#pragma once

#include <cstddef>
#include <vector>

#include "os/hxstr.h"

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
 * The object is 0x3c bytes with the vptr at `+0x38`. A leaf class with no base places its vptr
 * after its data members under this toolchain, which is why the pointer is last rather than first.
 * Its four vectors are recovered from the destructor at `0x00127158`, which tears them down at
 * `+0x2c`, `+0x1c`, `+0x10`, and `+0x04` and so fixes their declaration order as the reverse.
 * Element widths come from the shift each teardown uses to divide the byte span.
 *
 * The destructor body is empty. Everything the compiled destructor performs is the implicit
 * teardown of those four members, and the release of the object itself sits behind the deleting
 * flag this toolchain passes as a second argument. The declaration is real rather than implicit,
 * because a leaf class with no base acquires a virtual destructor only when one is declared
 * virtual.
 *
 * The class declares its own allocation pair. The release path of the destructor calls the tagged
 * free with the literal `PlayMap` at `0x007d0dd0`, and a tag names the class that declares the
 * operator.
 *
 * Every slot below whose verb is unrecovered keeps its table index as its title. The index is part
 * of the class layout, so a slot is declared whether or not its purpose is known, and the comment
 * records the behaviour that was recovered instead.
 */
class PlayMap {
public:
    /**
     * Allocate a map from the tagged heap under the tag `PlayMap`.
     *
     * Unlike most classes in this tree the operator has an out-of-line body, which forwards the
     * size the compiler supplies and the tag literal to the tagged allocator.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     * @ghidraAddress 0x00127118
     */
    void *operator new(size_t nSize);

    /**
     * Release a map to the tagged heap under the tag `PlayMap`.
     *
     * @param pBlock The block.
     * @ghidraAddress 0x00127138
     */
    void operator delete(void *pBlock);

    /** @ghidraAddress 0x00127158 */
    virtual ~PlayMap();

    /**
     * Slot 2. Appends one step, its distance from the previous step, and a label.
     *
     * The body appends `nValue - mSteps.back()` to mUnknown10, then nValue to mSteps, then the
     * label to mUnknown1c, so mUnknown10 stores the gap between consecutive steps and is always
     * one element behind mSteps until this call completes. Reading mSteps.back() is unconditional,
     * so the first call requires a step to already be present.
     *
     * The second parameter is passed by value. The body copy-constructs it into mUnknown1c through
     * HxStr::HxStr(const HxStr &) at `0x004b7b50` and then releases the parameter's own buffer,
     * which is this toolchain destroying a by-value class parameter in the callee.
     *
     * @param nValue The step position.
     * @param strLabel The label, passed by value.
     * @ghidraAddress 0x001268b0
     */
    virtual void Slot2(int nValue, HxStr strLabel);

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

    /**
     * Slot 5. Maps a position into the sequence, pure here and overridden by every subclass.
     *
     * The signature is recovered from both sides. Slots 12 and 13 forward their own argument
     * without touching a1 and then consume the result in v0, and each override reads that argument
     * and returns a value. PlayMapRing at `0x0012e408` returns `(nValue + mUnknown00) %
     * mSteps.back()`, which is the wrap its name implies.
     *
     * Slots 12 and 13 take the delta for this dispatch into a0 rather than a1, which is what
     * preserves the forwarded argument and is the reason the argument is recoverable at all.
     *
     * @param nValue The position to map.
     * @return The mapped position.
     */
    virtual int Slot5(int nValue) = 0;

    /**
     * Slot 6. Collects the positions of one span into mUnknown2c, pure here.
     *
     * The signature comes from PlayMapRing at `0x0012dad8`, which clears mUnknown2c, walks a value
     * from the first argument upward in steps of mSteps.back() while it remains below the third,
     * appends every value not below the second, and returns the vector itself. The return is the
     * address of the member, which is a reference in the original.
     *
     * @param nStart The first position.
     * @param nMin The lowest position to collect.
     * @param nEnd The position to stop below.
     * @return mUnknown2c.
     */
    virtual std::vector<int> &Slot6(int nStart, int nMin, int nEnd) = 0;

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

    /**
     * Slot 12. Reports the index of the step at or before the position slot 5 returns.
     *
     * The body forwards its argument to slot 5 through the table, searches mSteps for the value
     * that returns with an upper bound, and reports the distance from the start to the element
     * before the result. Slot 5 is pure here, so the search key comes from whichever subclass is
     * running.
     *
     * @param nValue The position to map through slot 5.
     * @return The step index.
     * @ghidraAddress 0x001276a0
     */
    virtual int Slot12(int nValue);

    /**
     * Slot 13. Reports the same step index with a multiple of the last step folded in.
     *
     * The body performs the search slot 12 performs, forwarding the same argument to slot 5, and
     * then adds `nTotal * (nValue - nValue % nTotal)` to the index, where nTotal is mSteps.back().
     * Multiplying by nTotal after rounding nValue down to a multiple of nTotal scales the term by
     * nTotal twice, which reads as an error and is what both the disassembly and the decompiler
     * agree the binary computes.
     *
     * @param nValue The position to map through slot 5 and to fold in.
     * @return The step index plus the folded term.
     * @ghidraAddress 0x00127700
     */
    virtual int Slot13(int nValue);

    /**
     * Slot 14. Returns zero.
     *
     * @ghidraAddress 0x00127460
     */
    virtual int Slot14();

    /**
     * Slot 15. Empty in this class, and it takes an argument the empty body cannot reveal.
     *
     * The body is `jr ra` and reads no argument register, which is no evidence about the parameter
     * list. PlayMapRepeatRing's override at `0x0012bc48` multiplies a1 by a step gap before storing
     * the result, so the member takes an int.
     *
     * @param nValue The multiplier the override applies to a step gap.
     * @ghidraAddress 0x00127468
     */
    virtual void Slot15(int nValue);

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

    /**
     * Report the index of the step at or before a position that has already been mapped.
     *
     * Slot12() performs the same search on the result of Slot5(). This routine receives the
     * mapped position directly. PhraseDatabase::GetStepValue() is the recovered caller.
     *
     * @param nPosition The mapped position.
     * @return The index of the last step at or before nPosition, or -1 when every step follows it.
     * @ghidraAddress 0x00127490
     */
    int FindStepIndex(int nPosition);

protected:
    // Declared in recovered offset order. Written by Slot3 and read nowhere yet recovered.
    int mUnknown00; // +0x00

public:
    /**
     * Ascending sequence of positions.
     *
     * Slots 8, 10, 12, and 13 read the last element, the count, and an upper bound over this
     * vector, and slot 2 appends to it. The element type is int, from the four-byte stride of every
     * access. Public because the PhraseDatabase constructor at `0x001b72d8` reads its last element
     * and its count directly and the image exposes no accessor. A friend declaration fits the image
     * equally well. +0x04
     */
    std::vector<int> mSteps;

protected:
    std::vector<int> mUnknown10;   // +0x10
    std::vector<HxStr> mUnknown1c; // +0x1c
    int mUnknown28;                // +0x28
    std::vector<int> mUnknown2c;   // +0x2c
};
