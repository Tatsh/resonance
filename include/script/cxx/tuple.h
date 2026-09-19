#pragma once

#include "script/cxx/config.h"
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
 * | 3 | inherited, unidentified | `0x0012ad48` |
 * | 4 | inherited, unidentified | `0x0012b378` |
 * | 5 | inherited, unidentified | `0x0012b3a0` |
 * | 6 | inherited, unidentified | `0x0012b358` |
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
