#pragma once

#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/standarderror.h"

namespace Py {

/**
 * Exception for a value of the wrong Python type, mirroring Python's TypeError.
 *
 * `Q22Py9TypeError` in the RTTI descriptor at `0x008ef380`, with Py::StandardError at offset 0 as
 * its one base. Its accessor is at `0x00148458`, and the emission at `0x004c69e0` is an
 * unreferenced copy.
 *
 * The object is one byte and stores nothing. The reason string is pushed into the CPython error
 * indicator by the constructor and is not retained, so a handler recovers it through
 * PyErr_Fetch() rather than from the thrown object. Py::Object::validate() at `0x004c3d10` is the
 * recovered throw site, and it proves both the constructor body below and the global that holds
 * `PyExc_TypeError`.
 */
class TypeError : public StandardError {
public:
    /**
     * Set the Python error indicator and become the thrown object.
     *
     * @param reason Text for the Python exception value. An empty string arrives at the
     *               interpreter as g_szEmptyString rather than as a null pointer.
     */
    TypeError(const HxStr &reason) {
        PyErr_SetString(PyExc_TypeError, reason.mStr != nullptr ? reason.mStr : g_szEmptyString);
    }
};

} // namespace Py
