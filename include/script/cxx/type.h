#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on a Python type object.
 *
 * `Q22Py4Type` in the RTTI descriptor at `0x00901d70`, with Py::Object at offset 0 as its one
 * base. Its accessor is at `0x004c60d0`.
 *
 * The vtable at `0x00821d78` has three entries, the same shape as Py::Object.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x004c60d0` |
 * | 1 | compiler-generated destructor | `0x004c6058` |
 * | 2 | `accepts` | `0x004c7058` |
 */
class Type : public Object {
public:
    /**
     * Accept only a type object.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a type object.
     * @ghidraAddress 0x004c7058
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyType_Check(pyob);
    }
};

} // namespace Py
