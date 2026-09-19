#pragma once

class HxStr;

/**
 * Mix-in for a screen that receives text from the front-end keyboard.
 *
 * `9MetKBUser` in the RTTI descriptor at `0x0086f598`, a leaf class with no base. The class
 * declares no data member, so the subobject is the four bytes of the compiler-generated vptr at
 * offset 0, which MetPersonaSaverScreen fixes by placing MemcardUser at `+140` and MetKBUser at
 * `+144`.
 *
 * Four classes derive from the class, MetFreqMakerButtonsScreen and MetPersonaSaverScreen and
 * MetSaveRemix at `+144`, and MetLoadNewFreqScreen at `+164`.
 *
 * The three-entry vtable at `0x007f16a0` runs GetTypeInfo, the destructor, then one entry that
 * stores the `__pure_virtual` handler at `0x005381a8`. The one declared virtual is therefore pure
 * and the original class is abstract. Its name is not recovered and its signature now is, so it is
 * declared below.
 *
 * The destructor at `0x0025dfb8` restores the vptr and releases the object through the scalar
 * release path at `0x004a9230` when its `__in_chrg` argument is odd, which is the whole of its
 * body.
 */
class MetKBUser {
public:
    /**
     * @ghidraAddress 0x0025dfb8
     */
    virtual ~MetKBUser();

    /**
     * Receive the text the keyboard screen committed. Slot 2.
     *
     * The name is not recovered. The one parameter is, from three of the overrides. The
     * MetSaveRemix override at `0x0037a650` passes it to HxStr::Assign as the source of an
     * assignment into its own `+0xb8`, which fixes it as a `const HxStr &` rather than an integer
     * or an object pointer. The MetSaveRemixScreen override at `0x00381990` forwards the same
     * register to a Rnd::Text through Rnd::Text::SetText(), and the MetRemixDelScreen override at
     * `0x00344240` forwards it to the MetSaveRemix override unchanged. Three of the seven overrides
     * in the image were read.
     *
     * @param text The text the user entered.
     * @ghidraAddress 0x005381a8
     */
    virtual void OnUnknownSlot2(const HxStr &text) = 0;
};
