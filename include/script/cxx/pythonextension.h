#pragma once

#include "script/cxx/pythonextensionbase.h"

namespace Py {

/**
 * Curiously recurring base that gives a C++ class its Python extension type.
 *
 * One instantiation exists in the image,
 * `Q22Pyt15PythonExtension1ZQ22Py22ExtensionModuleBasePtr`, the descriptor at `0x009029f0`,
 * deriving from Py::PythonExtensionBase at offset 0. The harvest does not demangle the name,
 * because its demangler does not handle the template form, so the name comes from the mangled
 * field instead, and the mangled string sits at `0x00833450`.
 *
 * The one derived class is Py::ExtensionModuleBasePtr, which passes itself as the parameter, so
 * the instantiation is the usual recurring-template shape.
 *
 * No member routine is identified, so none is reconstructed.
 */
template <typename T>
class PythonExtension : public PythonExtensionBase {};

} // namespace Py
