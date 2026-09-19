#pragma once

namespace Py {

/**
 * Builder for the CPython type object of an extension type.
 *
 * `Q22Py10PythonType` in the RTTI descriptor at `0x0086f588`, with no base. Its accessor is at
 * `0x005aba50`, and the mangled name string sits at `0x00833400`.
 *
 * Nothing beyond the descriptor is recovered. Its vtable was not located and no member routine is
 * identified, so the declaration records the class and nothing more. Released PyCXX gives it a
 * `PyTypeObject` member plus a long run of chained setters, and none of those survives as a
 * routine of its own here.
 */
class PythonType {};

} // namespace Py
