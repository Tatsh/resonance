#pragma once

#include "script/cxx/config.h"

namespace Py {

/**
 * Temporary owner of a new reference returned by the CPython API.
 *
 * `Q22Py7FromAPI` in the RTTI descriptor at `0x0086f6b8`, with no base. The vtable at `0x007d1220`
 * has two entries, the compiler-generated type function at `0x0012b548` and the destructor at
 * `0x0012b588`. The object is eight bytes, the reference at `+0x00` and the vptr after it at
 * `+0x04`, which is where this compiler places the vptr of a class with no base.
 *
 * This class is how the port adopts an owned reference, and it replaces the `owned` constructor
 * flag of released PyCXX, which the image does not have. Every recovered adoption follows one
 * shape. A FromAPI temporary receives the new reference, a Py::Object is built or assigned from
 * `mPtr` and adds a count of its own, and the temporary releases the extra count at the end of
 * the full expression. Py::Dict::Dict() and PyShell::Eval() are the two clearest instances.
 *
 * The destructor releases the reference and does not clear the member, unlike
 * Py::Object::release(), which clears it. Since the object is always a temporary, the difference
 * is unobservable.
 */
class FromAPI {
public:
    /**
     * Adopt a new reference.
     *
     * @param pyob The reference to adopt, which may be null.
     */
    explicit FromAPI(PyObject *pyob) : mPtr(pyob) {
    }

    /**
     * Release the adopted reference.
     *
     * @ghidraAddress 0x0012b588
     */
    virtual ~FromAPI() {
        Py_XDECREF(mPtr);
    }

    /**
     * Adopted reference, null when the API call failed.
     *
     * Public because every consumer reads it directly and the image has no accessor for it. A
     * trivial accessor and a public member compile to the same single load, so the evidence
     * cannot distinguish them, and the public member adds no function that no address can be
     * attached to.
     *
     * +0x00
     */
    PyObject *mPtr;
};

} // namespace Py
