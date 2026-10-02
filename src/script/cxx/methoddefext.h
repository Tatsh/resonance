#pragma once

#include "script/cxx/config.h"

namespace Py {

/**
 * Method record of an extension type, keyed by name in PythonExtension::methods().
 *
 * The class does not emit an RTTI descriptor. The name is therefore released PyCXX's rather than
 * the image's. PythonExtension::getattr_methods() at `0x005ad5e0` hands `PyCFunction_New()` the
 * record `0x10` bytes in, past the inherited `PyMethodDef`. The offset places the member below.
 * Released PyCXX follows the member with the C++ member pointers the call handlers dispatch
 * through. The image does not register a method on any extension type, and the member pointers
 * are not reconstructed.
 *
 * @tparam T The extension type.
 */
template <typename T>
class MethodDefExt : public PyMethodDef {
public:
    /** The record handed to the interpreter. */
    PyMethodDef mExtMethodDef;
};

} // namespace Py
