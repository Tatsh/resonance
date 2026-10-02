#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on a one-character Python string, and the element type of Py::String.
 *
 * `Q22Py4Char` in the RTTI descriptor at `0x009020d0`, with Py::Object at offset 0 as its one
 * base. Its accessor is at `0x004c7cc8`.
 *
 * The vtable at `0x00821f90` has three entries, the same shape as Py::Object.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x004c7cc8` |
 * | 1 | compiler-generated destructor | `0x004c7ca8` |
 * | 2 | `accepts` | `0x004c7d40` |
 */
class Char : public Object {
public:
    /**
     * Take a borrowed reference to an existing one-character string.
     *
     * Recovered from `Py::SeqBase<Py::Char>::getItem()` at `0x004c7a48`, which builds the
     * returned element this way. The body is inlined at that site and has no address of its own.
     *
     * @param pyob The string to wrap.
     */
    explicit Char(PyObject *pyob) : Object(pyob) {
        validate();
    }

    /**
     * Accept only a string of length one.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a one-character string.
     * @ghidraAddress 0x004c7d40
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyString_Check(pyob) && PySequence_Length(pyob) == 1;
    }
};

} // namespace Py
