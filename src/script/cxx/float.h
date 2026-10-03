#pragma once

#include "script/cxx/config.h"
#include "script/cxx/object.h"

namespace Py {

/**
 * Handle on a Python float.
 *
 * Its RTTI descriptor is at `0x00902170`. It has Py::Object at offset 0 as its one base. Its
 * accessor is at `0x00454290` and its destructor at `0x00454218`.
 *
 * Three translation units emit a copy of its vtable, the same shape as Py::Object, and each copy's
 * accepts() slot points at the one body at `0x0040c418`. The slots are at `0x0081734c`,
 * `0x00819464`, and `0x0081b9f4`, beside the type function copy at `0x0040c3a0`.
 */
class Float : public Object {
public:
    /**
     * Take a borrowed reference to an existing float.
     *
     * Inline. Every script command that converts an argument with `PyNumber_Float()` stores the new
     * reference in a Py::FromAPI temporary and builds the handle this way, ScriptFadeActivator()
     * at `0x0044b7a0` among them.
     *
     * @param pyob The float to wrap.
     */
    explicit Float(PyObject *pyob) : Object(pyob) {
        validate();
    }

    /**
     * Accept only a float.
     *
     * @param pyob The reference to test.
     * @return True when the reference is a float.
     * @ghidraAddress NTSC-U/C: 0x0040c418
     * @ghidraAddress PAL: 0x00445e40
     */
    virtual bool accepts(PyObject *pyob) const {
        return pyob != nullptr && PyFloat_Check(pyob);
    }
};

} // namespace Py
