#pragma once

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
 * and the original class is abstract. Its name and signature are not recovered, so it is recorded
 * here rather than declared, and this declaration is consequently instantiable where the original
 * was not.
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
};
