#pragma once

/**
 * Mix-in for an object that takes part in a screen fade.
 *
 * `8FadeUser` in the RTTI descriptor at `0x0086f758`, a leaf class with no base. The class
 * declares no data member, so the subobject is the four bytes of the compiler-generated vptr at
 * offset 0. MetMCFreqDelScreen fixes that width for a mix-in of this shape, placing MemcardUser at
 * `+140` and ListDataProvider at `+144`, and the same construction applies here.
 *
 * Five classes derive from the class. MetExpansionPakScreen, MetLoadGameScreen, and MetSonyScreen
 * place the subobject at `+140`, MetMemDetectStartup at `+160`, and MetRenderer at `+92`.
 *
 * The four-entry vtable at `0x007ec070` runs GetTypeInfo, the destructor, then two entries that
 * both store the `__pure_virtual` handler at `0x005381a8`. Both declared virtuals are therefore
 * pure and the original class is abstract. Neither one has a recovered name or signature, because
 * the foundation classes invoke neither, so the two are recorded here rather than declared. This
 * declaration is consequently instantiable where the original was not.
 *
 * The destructor at `0x0021d760` restores the vptr and releases the object through the scalar
 * release path at `0x004a9230` when its `__in_chrg` argument is odd, which is the whole of its
 * body. Construction is inline, and every derived constructor in the image writes the vptr in
 * place rather than calling out.
 */
class FadeUser {
public:
    /**
     * @ghidraAddress 0x0021d760
     */
    virtual ~FadeUser();
};
