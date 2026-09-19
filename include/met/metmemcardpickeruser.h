#pragma once

/**
 * Mix-in for a screen that receives the outcome of the memory-card picker.
 *
 * `20MetMemCardPickerUser` in the RTTI descriptor at `0x0086f768`, a leaf class with no base. The
 * class declares no data member, so the subobject is the four bytes of the compiler-generated
 * vptr at offset 0. MetMCFreqDelScreen fixes that width, listing MemcardUser at `+140`,
 * ListDataProvider at `+144`, and MetMemCardPickerUser at `+148`.
 *
 * Three classes derive from the class, MetMCFreqDelScreen at `+148`, MetMemCardLoadScreen at
 * `+160`, and MetRemixDelScreen at `+236`.
 *
 * No vtable for the class is located. Its descriptor is built by three GetTypeInfo copies at
 * `0x002c6090`, `0x002d2378`, and `0x003446c0`, and a scan of the whole read-only data region
 * finds no vtable entry whose function pointer is any of the three, and no secondary vtable run
 * whose `this` adjustment is `-148` or `-236`. The slot count, the destructor address, and every
 * declared virtual are therefore undetermined, and the class is recorded here at the width its
 * derived classes prove rather than reconstructed. Do not treat the destructor below as an
 * observed address. It is declared because a class with a vptr has one, and no address is
 * asserted for it.
 */
class MetMemCardPickerUser {
public:
    virtual ~MetMemCardPickerUser();
};
