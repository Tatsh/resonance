#pragma once

namespace Py {

/**
 * Root of the PyCXX exception hierarchy.
 *
 * `Q22Py9Exception` in the RTTI descriptor at `0x0086f5c8`, with no base. The descriptor exists
 * for the exception tables rather than for polymorphism, because the class has no vtable anywhere
 * in the image.
 *
 * The class is empty, and that is measured rather than assumed. Every `throw Exception()` in the
 * image reserves one byte for the thrown object and writes no field into it. Py::Object::validate()
 * at `0x004c3d10` is the clearest instance. The CPython error indicator carries the detail, so the
 * thrown object needs no state.
 *
 * Released PyCXX gives this class a constructor taking a module and a name, plus a `clear()`
 * helper. Neither survives in the image, so neither is reconstructed.
 */
class Exception {};

} // namespace Py
