#pragma once

#include "script/cxx/config.h"
#include "script/cxx/exception.h"
#include "script/cxx/object.h"
#include "script/cxx/seqbase.h"

namespace Py {

/**
 * Handle on a Python tuple.
 *
 * `Q22Py5Tuple` in the RTTI descriptor at `0x00901d80`, with `Py::SeqBase<Py::Object>` at offset 0
 * as its one base. Its accessor is at `0x004c67e8`.
 *
 * The vtable at `0x00821c18` has nine entries, and only the type function, the destructor, accepts
 * (), and slot 8 differ from the base table.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x004c67e8` |
 * | 1 | compiler-generated destructor | `0x004c6770` |
 * | 2 | `accepts` | `0x004c75c0` |
 * | 3 | inherited `max_size` | `0x0012ad48` |
 * | 4 | inherited `capacity` | `0x0012b378` |
 * | 5 | inherited `swap` | `0x0012b3a0` |
 * | 6 | inherited `size` | `0x0012b358` |
 * | 7 | inherited `getItem` | `0x0012ac48` |
 * | 8 | `setItem` | `0x004c7428` |
 */
class Tuple : public SeqBase<Object> {
public:
    /**
     * Take a borrowed reference to an existing tuple.
     *
     * The routine takes two arguments and no third. Released PyCXX gives the same constructor an
     * `owned` flag, and this build has none, which is one of the two established differences
     * between the shipped binding and the release.
     *
     * @param pyob The tuple to wrap.
     * @ghidraAddress 0x004c5448
     */
    explicit Tuple(PyObject *pyob) : SeqBase<Object>(pyob) {
        validate();
    }

    /**
     * Create a tuple of a given length.
     *
     * Only the signature is recovered. The body builds a handle on the result of a zero-length
     * `PyTuple_New()` and then grows it, and the growth path is not worked out, so no body is
     * written. PyShell::ReportError() creates a two-element tuple this way.
     *
     * @param nSize The number of elements.
     * @ghidraAddress 0x004c5690
     */
    explicit Tuple(int nSize);

    /**
     * Take another handle's reference as a tuple.
     *
     * The body is out of line. It belongs to the vendored binding and is not reconstructed.
     *
     * @param ob The handle to copy.
     * @ghidraAddress 0x004c5568
     */
    explicit Tuple(const Object &ob);

    /**
     * Write one element.
     *
     * The override exists for the reference count. `PyTuple_SetItem()` steals a reference, so the
     * body adds one before the call, where the base's `PySequence_SetItem()` path borrows and
     * needs no such step.
     *
     * @param i The index.
     * @param value The element to store.
     * @ghidraAddress 0x004c7428
     */
    virtual void setItem(int i, const Object &value) {
        Py_XINCREF(value.mPtr);
        if (PyTuple_SetItem(mPtr, i, value.mPtr) == -1) {
            throw Exception();
        }
    }

    /**
     * Accept only a tuple.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a tuple.
     * @ghidraAddress 0x004c75c0
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyTuple_Check(pyob);
    }
};

} // namespace Py
