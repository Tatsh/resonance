#pragma once

#include "os/hxstr.h"
#include "script/cxx/config.h"
#include "script/cxx/standarderror.h"

namespace Py {

/**
 * Exception for an unresolved name, mirroring Python's NameError.
 *
 * `Q22Py9NameError` in the RTTI descriptor at `0x008ef390`, with Py::StandardError at offset 0 as
 * its one base. Its accessor is at `0x0015feb0`, and the emission at `0x005ae670` is an
 * unreferenced copy. The `hx.test` command throws it for an unknown test name.
 */
class NameError : public StandardError {
public:
    /**
     * Set the Python error indicator and become the thrown object.
     *
     * @param reason Text for the Python exception value. An empty string arrives at the
     *               interpreter as g_szEmptyString rather than as a null pointer.
     */
    NameError(const HxStr &reason) {
        PyErr_SetString(PyExc_NameError, reason.mStr != nullptr ? reason.mStr : g_szEmptyString);
    }
};

} // namespace Py
