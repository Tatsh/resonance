#pragma once

#include "script/cxx/pythonextension.h"

namespace Py {

/**
 * Python-visible wrapper that points back at an extension module's C++ object.
 *
 * `Q22Py22ExtensionModuleBasePtr` in the RTTI descriptor at `0x00902360`, with
 * `Py::PythonExtension<Py::ExtensionModuleBasePtr>` at offset 0 as its one base. Its accessor is
 * at `0x005abdb8`, and the mangled name string sits at `0x00833488`.
 *
 * The class passes itself as its base's template parameter, which is how PyCXX gives an extension
 * type its own Python type object. Released PyCXX hides the same wrapper inside `Extensions.hxx`
 * so that a module method can recover the C++ module object from the `self` argument.
 *
 * No member routine is identified, so none is reconstructed.
 */
class ExtensionModuleBasePtr : public PythonExtension<ExtensionModuleBasePtr> {};

} // namespace Py
