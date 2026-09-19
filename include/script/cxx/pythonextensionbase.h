#pragma once

#include "script/cxx/config.h"

namespace Py {

/**
 * Base of every C++ object the binding exposes to Python as an extension object.
 *
 * `Q22Py19PythonExtensionBase` in the RTTI descriptor at `0x008effc0`, with `_object` at offset 0
 * as its one base. Its accessor is at `0x005aba90`, and the mangled name string sits at
 * `0x00833418`.
 *
 * The base is the interpreter's own `PyObject` structure, which is what makes an instance usable
 * from Python without a separate header block. Released PyCXX instead places the header inside
 * `PythonExtension<T>` through the `PyObject_HEAD` macro, so the inheritance here is a difference
 * between the shipped binding and the release rather than a reading of it.
 *
 * `_object` has its own descriptor at `0x0086f6a8`, with an accessor at `0x005ae738`. That
 * descriptor belongs to the interpreter and is excluded from reconstruction.
 *
 * No member routine of this class is identified, so none is reconstructed.
 */
class PythonExtensionBase : public _object {};

} // namespace Py
