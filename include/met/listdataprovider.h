#pragma once

/**
 * Mix-in for an object that supplies the rows of a scrolling list to the screen that draws it.
 *
 * `16ListDataProvider` in the RTTI descriptor at `0x0086f760`, a leaf class with no base. The
 * class declares no data member, so the subobject is the four bytes of the compiler-generated vptr
 * at offset 0. MetMCFreqDelScreen proves the width directly, listing MemcardUser at `+140`,
 * ListDataProvider at `+144`, and MetMemCardPickerUser at `+148`.
 *
 * Five classes derive from the class. MetFreqMakerDirectionsScreen, MetJukeboxBaseScreen, and
 * MetRemixLoadScreen place the subobject at `+140`, MetMCFreqDelScreen at `+144`, and
 * MetRemixDelScreen at `+232`.
 *
 * The four-entry vtable at `0x007ec830` runs GetTypeInfo, the destructor, then two entries that
 * both store the `__pure_virtual` handler at `0x005381a8`. Both declared virtuals are therefore
 * pure and the original class is abstract. MetJukeboxBaseScreen supplies the two at `0x0021ef30`
 * and `0x00224ae8` through the secondary vtable at `0x007ec6a8`, whose every entry adjusts `this`
 * by `-140` back to the start of the screen. Neither virtual has a recovered name or signature, so
 * the two are recorded here rather than declared, and this declaration is consequently
 * instantiable where the original was not.
 *
 * The destructor at `0x002247f0` restores the vptr and releases the object through the scalar
 * release path at `0x004a9230` when its `__in_chrg` argument is odd, which is the whole of its
 * body.
 */
class ListDataProvider {
public:
    /**
     * @ghidraAddress 0x002247f0
     */
    virtual ~ListDataProvider();
};
