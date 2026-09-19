#pragma once

#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/standarderror.h"

namespace Py {

/**
 * Exception for a binding rule broken at run time, mirroring Python's RuntimeError.
 *
 * `Q22Py12RuntimeError` in the RTTI descriptor at `0x008ef078`, with Py::StandardError at offset 0
 * as its one base. Its accessor is at `0x005abc48`.
 *
 * Py::MethodTable::add() at `0x005a5bc8` throws it with `Too late to add a module method!`, and
 * the string `Object::decrement_reference_count error.` at `0x00821bb8` belongs to the second
 * PyCXX site that released PyCXX guards the same way. The constructor body and the global holding
 * `PyExc_RuntimeError` both come from the MethodTable site.
 */
class RuntimeError : public StandardError {
public:
    /**
     * Set the Python error indicator and become the thrown object.
     *
     * @param reason Text for the Python exception value. An empty string arrives at the
     *               interpreter as g_szEmptyString rather than as a null pointer.
     */
    RuntimeError(const HxStr &reason) {
        PyErr_SetString(PyExc_RuntimeError, reason.mStr != nullptr ? reason.mStr : g_szEmptyString);
    }
};

} // namespace Py
