#pragma once

namespace Py {

/**
 * Base of an extension module the binding registers with the interpreter.
 *
 * `Q22Py19ExtensionModuleBase` in the RTTI descriptor at `0x0086f650`, with no base. Its accessor
 * is at `0x005aba10`.
 *
 * Nothing beyond the descriptor is recovered. Its vtable was not located and no member routine is
 * identified. The game does not appear to use the class for its own `hx` module either, because
 * InitHxModule() at `0x005597b8` drives `Py_InitModule4()` directly from a Py::MethodTable rather
 * than through a module object.
 */
class ExtensionModuleBase {};

} // namespace Py
