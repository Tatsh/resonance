#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on any Python sequence, and the base of Py::List, Py::Tuple, and Py::String.
 *
 * Two instantiations exist in the image. `Q22Pyt7SeqBase1ZQ22Py6Object` is the descriptor at
 * `0x00902990`, and `Q22Pyt7SeqBase1ZQ22Py4Char` is the descriptor at `0x00902000`. Both derive
 * from Py::Object at offset 0. The harvest demangles neither, because its demangler does not
 * handle the template form, so both names come from the mangled field instead.
 *
 * The vtable has nine entries, the three Py::Object slots plus six of its own. Four of the six
 * are unidentified, and the table below records their addresses so that a later pass can resume
 * from them rather than rediscover them. The two that are identified are declared below, and they
 * therefore appear here in an order that does not match the table.
 *
 * | Slot | Member | `SeqBase<Object>` | `SeqBase<Char>` |
 * | ---- | ------ | ----------------- | --------------- |
 * | 0 | compiler-generated type function | `0x0012abd0` | `0x004c64e8` |
 * | 1 | inherited destructor | `0x0012ab58` | `0x004c6470` |
 * | 2 | `accepts` | `0x0012b328` | `0x004c7c30` |
 * | 3 | unidentified | `0x0012ad48` | `0x004c7260` |
 * | 4 | unidentified | `0x0012b378` | `0x004c7c80` |
 * | 5 | unidentified | `0x0012b3a0` | `0x004c7890` |
 * | 6 | unidentified | `0x0012b358` | `0x004c73d0` |
 * | 7 | `getItem` | `0x0012ac48` | `0x004c7a48` |
 * | 8 | `setItem` | `0x0012b4d0` | `0x004c7bb8` |
 *
 * The two vtables for `SeqBase<Object>` at `0x007d0ff0`, `0x00821c68`, and `0x00825878` are
 * identical copies that the linker did not fold. `SeqBase<Char>` sits at `0x00821d08`.
 */
template <typename T>
class SeqBase : public Object {
protected:
    /**
     * Produce a handle on `None` for a derived class that fills it in afterwards.
     *
     * The body is empty, which is what the folded vptr write in a derived default constructor
     * establishes. This compiler omits the intermediate vptr store when the intermediate
     * constructor runs no code between the two stores.
     */
    SeqBase() {
    }

    /**
     * Take a borrowed reference on behalf of a derived class.
     *
     * Recovered from Py::Tuple's constructor at `0x004c5448`, where the three vptr stores and the
     * three validate() calls of the chain are all visible in one routine.
     *
     * @param pyob The reference to wrap.
     */
    explicit SeqBase(PyObject *pyob) : Object(pyob) {
        validate();
    }

public:
    /**
     * Accept any reference the sequence protocol supports.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a sequence.
     * @ghidraAddress 0x0012b328
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PySequence_Check(pyob) != 0;
    }

    /**
     * Read one element.
     *
     * Slot 7 of the table above. The body is not recovered. The signature comes from the call in
     * PyShell::ReportError(), which passes a hidden return slot, the adjusted receiver, and the
     * index.
     *
     * @param i The index.
     * @return A handle on the element.
     * @ghidraAddress 0x0012ac48
     */
    virtual T getItem(int i) const;

    /**
     * Write one element.
     *
     * Slot 8 of the table above. The body is not recovered.
     *
     * @param i The index.
     * @param value The element to store.
     * @ghidraAddress 0x0012b4d0
     */
    virtual void setItem(int i, const T &value);
};

} // namespace Py
