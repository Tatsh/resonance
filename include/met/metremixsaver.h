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
 * The six-entry vtable at `0x007ffcd0` runs GetTypeInfo, the destructor, then three entries that
 * all store the `__pure_virtual` handler at `0x005381a8`. All three declared virtuals are
 * therefore pure and the original class is abstract. None has a recovered name or signature, so the
 * three are recorded here rather than declared, and this declaration is consequently instantiable
 * where the original was not.
 *
 * The destructor at `0x002fecf0` restores the vptr and releases the object through the scalar
 * release path at `0x004a9230` when its `__in_chrg` argument is odd, which is the whole of its
 * body.
 */
class MetRemixSaver {
public:
    /**
     * @ghidraAddress 0x002fecf0
     */
    virtual ~MetRemixSaver();
};
