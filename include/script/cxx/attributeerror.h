#pragma once

#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/standarderror.h"

namespace Py {

/**
 * Exception for a missing attribute, mirroring Python's AttributeError.
 *
 * `Q22Py14AttributeError` in the RTTI descriptor at `0x008eefb8`, with Py::StandardError at offset
 * 0 as its one base. Its accessor is at `0x004c6b28`. PythonExtension::getattr_methods() at
 * `0x005ad5e0` throws it for a name with no method.
 */
class AttributeError : public StandardError {
public:
    /**
     * Set the Python error indicator and become the thrown object.
     *
     * @param reason Text for the Python exception value. An empty string arrives at the
     *               interpreter as g_szEmptyString rather than as a null pointer.
     */
    AttributeError(const HxStr &reason) {
        PyErr_SetString(PyExc_AttributeError,
                        reason.mStr != nullptr ? reason.mStr : g_szEmptyString);
    }
};

} // namespace Py
