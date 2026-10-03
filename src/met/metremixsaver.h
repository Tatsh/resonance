#pragma once

/**
 * Mix-in for a screen that writes a remix to a memory card.
 *
 * Its RTTI descriptor is at `0x0086f5a8`. It is a leaf class with no base. The class declares no
 * data member. The subobject is therefore the four-byte compiler-generated vptr at offset 0, the
 * same width that MetMCFreqDelScreen proves for a mix-in of this shape.
 *
 * Two classes derive from the class, MetMultiSaveRemixScreen and MetSoloEndRemixScreen, both
 * placing the subobject at `+140`.
 *
 * The vtable at `0x007ffcd0` has five entries. Slot 0 is the compiler-generated GetTypeInfo at
 * `0x002fecb0` and is not source. Slot 1 is the destructor, and slots 2, 3, and 4 all store the
 * `__pure_virtual` handler at `0x005381a8`, so all three declared virtuals are pure and the class
 * is abstract.
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
     * @ghidraAddress NTSC-U/C: 0x002fecf0
     * @ghidraAddress PAL: 0x00323240
     */
    virtual ~MetRemixSaver();

    /**
     * Continue once the save screen has finished. Slot 2.
     *
     * MetSaveRemixScreen runs it with 0 when the save is abandoned or the user backs out, and with
     * 1 when a save dialogue it does not handle itself closes. The MetSoloEndRemixScreen override
     * at `0x003998c8` tests one member and dispatches through the primary table, and the
     * MetMultiSaveRemixScreen override at `0x002facc0` opens the next save. Neither reads an
     * argument register.
     *
     * @param bCompleted 1 after a save dialogue closed, 0 otherwise. Neither override reads it.
     * @ghidraAddress NTSC-U/C: 0x005381a8
     * @ghidraAddress PAL: 0x00577a68
     */
    virtual void OnSaveFinished(int bCompleted) = 0;

    /**
     * Note that the user exited the save screen for the help screen. Slot 3.
     *
     * Both overrides, at `0x002fee48` and `0x00399898`, write one to the member the slot 4 override
     * clears and then dispatch through the primary table. Neither reads an argument register.
     *
     * @ghidraAddress NTSC-U/C: 0x005381a8
     * @ghidraAddress PAL: 0x00577a68
     */
    virtual void OnHelpRequested() = 0;

    /**
     * Push or exit the saver's screen as the save screen returns or is declined. Slot 4.
     *
     * Both overrides, at `0x002fb248` and `0x00395918`, write the logical negation of the argument
     * into a member and branch on the same argument.
     *
     * @param bShowing 1 when the save screen returns, 0 when the save is declined.
     * @ghidraAddress NTSC-U/C: 0x005381a8
     * @ghidraAddress PAL: 0x00577a68
     */
    virtual void SetOwnerScreenShowing(int bShowing) = 0;
};
