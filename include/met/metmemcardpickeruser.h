#pragma once

/**
 * Mix-in for a screen that receives the outcome of the memory-card picker.
 *
 * `20MetMemCardPickerUser` in the RTTI descriptor at `0x0086f768`, a leaf class with no base. The
 * subobject is four bytes, which MetMCFreqDelScreen fixes by listing MemcardUser at `+140`,
 * ListDataProvider at `+144`, and MetMemCardPickerUser at `+148`.
 *
 * Alone among the mix-ins of this subsystem the class declares no virtual function, so it has no
 * vptr and the four bytes are one data member. Four independent observations agree on that.
 *
 *  - No vtable anywhere in the image references any of the three GetTypeInfo copies at
 *    `0x002c6090`, `0x002d2378`, and `0x003446c0`.
 *  - A scan of the whole read-only data region finds no secondary vtable run whose `this`
 *    adjustment is `-148`, `-160`, or `-236`, the three offsets its derived classes use.
 *  - MetMCFreqDelScreen emits two secondary vtables, for MemcardUser and ListDataProvider, and
 *    MetMemCardLoadScreen emits one, for the MemcardUser inside MetMemDetectScreen. Neither emits
 *    a third.
 *  - The MetRemixDelScreen constructor at `0x003394a0` writes four vptrs, at `+0x00`, `+0x8c`,
 *    `+0x90`, and `+0xe8`, and writes nothing at `+0xec` where its MetMemCardPickerUser subobject
 *    sits.
 *
 * The descriptor therefore exists only because three derived descriptors list the class as a base,
 * and the three accessors exist only because those three builders call them. The purpose and the
 * type of the one data member are not recovered, and no routine that reads it has been traced.
 *
 * Three classes derive from the class, MetMCFreqDelScreen at `+148`, MetMemCardLoadScreen at
 * `+160`, and MetRemixDelScreen at `+236`.
 */
class MetMemCardPickerUser {
private:
    int mUnknown00; // +0x00
};
