#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on any callable Python object.
 *
 * Its RTTI descriptor is at `0x00902a30`. It has Py::Object at offset 0 as its one base. Its
 * accessor is at `0x0050d2e8`.
 *
 * The vtable at `0x008257f8` has three entries, the same shape as Py::Object.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x0050d2e8` |
 * | 1 | compiler-generated destructor | `0x0050d270` |
 * | 2 | `accepts` | `0x0050d360` |
 *
 * Released PyCXX also gives this class `apply()` overloads for invoking the object. None appears
 * in the image as a routine of its own, and PyShell::ReportError() invokes `traceback_str` through
 * the plain interpreter call instead, so none is reconstructed.
 */
class Callable : public Object {
public:
    /**
     * Narrow an existing handle to a callable one.
     *
     * Recovered from PyShell::ReportError(), which narrows the `traceback_str` attribute of the
     * `hxutl` module this way. The body is inlined at that site and has no address of its own.
     *
     * @param other The handle to narrow.
     */
    explicit Callable(const Object &other) : Object(other) {
        validate();
    }

    /**
     * Accept only a callable object.
     *
     * @param pyob The reference to test.
     * @return True when the reference is callable.
     * @ghidraAddress NTSC-U/C: 0x0050d360
     * @ghidraAddress PAL: 0x0054c818
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyCallable_Check(pyob) != 0;
    }
};

} // namespace Py
