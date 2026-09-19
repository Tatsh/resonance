#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on a Python integer.
 *
 * `Q22Py3Int` in the RTTI descriptor at `0x008efd90`, with Py::Object at offset 0 as its one base.
 * Its accessor is at `0x004c63f8`.
 *
 * The vtable at `0x00821d58` has three entries, the same shape as Py::Object.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x004c63f8` |
 * | 1 | compiler-generated destructor | `0x004c6380` |
 * | 2 | `accepts` | `0x004c7218` |
 */
class Int : public Object {
public:
    /**
     * Accept only an integer.
     *
     * @param pyob The reference to test.
     * @return True when the reference is an integer.
     * @ghidraAddress 0x004c7218
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyInt_Check(pyob);
    }
};

} // namespace Py
