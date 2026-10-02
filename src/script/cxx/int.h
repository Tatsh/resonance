#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on a Python integer.
 *
 * Its RTTI descriptor is at `0x008efd90`. It has Py::Object at offset 0 as its one base. Its
 * accessor is at `0x004c63f8`.
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
     * Convert another handle's object to an integer.
     *
     * The handle starts on `None`, takes the result of `PyNumber_Int()` at `0x004a3438` through
     * set() from a Py::FromAPI temporary, releasing `None`, and runs validate() once more.
     *
     * @param ob The handle to convert.
     * @ghidraAddress 0x004c4680
     */
    explicit Int(const Object &ob);

    /**
     * Wrap the interpreter's 64-bit long as an integer.
     *
     * The handle starts on `None`, takes the result of `PyInt_FromLong()` at `0x00580c68` through
     * set() from a Py::FromAPI temporary, releasing `None`, and runs validate() once more.
     *
     * @param nValue The value to wrap.
     * @ghidraAddress 0x004c44c0
     */
    explicit Int(long long nValue);

    /**
     * Take another handle's object, converted to an integer.
     *
     * @param ob The handle to convert.
     * @return This handle.
     * @ghidraAddress 0x004c71f8
     */
    Int &operator=(const Object &ob);

    /**
     * Take a reference, converted to an integer.
     *
     * The handle takes the result of `PyNumber_Int()` through set() from a Py::FromAPI temporary,
     * unless the reference is the one it already has.
     *
     * @param pyob The reference to convert.
     * @return This handle.
     * @ghidraAddress 0x004c4840
     */
    Int &operator=(PyObject *pyob);

    /**
     * Take a value.
     *
     * The value is wrapped by `PyInt_FromLong()` and then converted again by the reference
     * assignment above.
     *
     * @param nValue The value.
     * @return This handle.
     * @ghidraAddress 0x004c4980
     */
    Int &operator=(int nValue);

    /**
     * Take a 64-bit value, the same way as the int overload.
     *
     * @param nValue The value.
     * @return This handle.
     * @ghidraAddress 0x004c4a60
     */
    Int &operator=(long long nValue);

    /**
     * Read the value.
     *
     * The out-of-line body forwards to `PyInt_AsLong()`. The result is the interpreter's 64-bit
     * long. Callers test the result at full width before any narrowing.
     *
     * @return The value.
     * @ghidraAddress 0x004c7240
     */
    operator long long() const;

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
