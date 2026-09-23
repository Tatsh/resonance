#pragma once

/**
 * Mix-in for a screen that writes a remix to a memory card.
 *
 * `13MetRemixSaver` in the RTTI descriptor at `0x0086f5a8`, a leaf class with no base. The class
 * declares no data member, so the subobject is the four bytes of the compiler-generated vptr at
 * offset 0, the same width that MetMCFreqDelScreen proves for a mix-in of this shape.
 *
 * Two classes derive from the class, MetMultiSaveRemixScreen and MetSoloEndRemixScreen, both
 * placing the subobject at `+140`.
 *
 * The vtable at `0x007ffcd0` has five entries. Slot 0 is the compiler-generated GetTypeInfo at
 * `0x002fecb0` and is not source. Slot 1 is the destructor, and slots 2, 3, and 4 all store the
 * `__pure_virtual` handler at `0x005381a8`, so all three declared virtuals are pure and the class
 * is abstract. An earlier reading counted six entries by including the terminator.
 *
 * The two derived tables at `0x007ffb60` and `0x0080c4c0` each have five entries, adjust `this` by
 * `-140` in every entry, and supply the three pure virtuals. Their signatures come from those six
 * bodies. Neither derived header declares its overrides yet.
 *
 * The destructor at `0x002fecf0` restores the vptr and releases the object through the scalar
 * release path at `0x004a9230` when its `__in_chrg` argument is odd, which is the whole of its
 * body. Both of those are compiler-generated, so the definition is empty.
 */
class MetRemixSaver {
public:
    /**
     * @ghidraAddress 0x002fecf0
     */
    virtual ~MetRemixSaver();

    /**
     * Unrecovered. Slot 2.
     *
     * The MetSoloEndRemixScreen override at `0x003998c8` tests one member and dispatches through
     * the primary table, and the MetMultiSaveRemixScreen override at `0x002facc0` runs for several
     * hundred instructions. Neither reads an argument register, but MetSaveRemixScreen's slots 40
     * and 41 pass 0 and 1 in a1, so the slot takes one integer that both overrides ignore.
     *
     * @param nUnknown Passed by the caller and read by neither override.
     * @ghidraAddress 0x005381a8
     */
    virtual void OnUnknownSlot2(int nUnknown) = 0;

    /**
     * Unrecovered. Slot 3.
     *
     * Both overrides, at `0x002fee48` and `0x00399898`, write one to the member the slot 4 override
     * clears and then dispatch through the primary table. Neither reads an argument register.
     *
     * @ghidraAddress 0x005381a8
     */
    virtual void OnUnknownSlot3() = 0;

    /**
     * Unrecovered. Slot 4.
     *
     * Both overrides, at `0x002fb248` and `0x00395918`, write the logical negation of one integer
     * argument into a member and branch on the same argument, which is what fixes the single
     * parameter.
     *
     * @param nFlag The value the override negates into its own member.
     * @ghidraAddress 0x005381a8
     */
    virtual void OnUnknownSlot4(int nFlag) = 0;
};
