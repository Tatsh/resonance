#pragma once

#include "script/cxx/config.h"
#include "script/cxx/dict.h"
#include "script/cxx/object.h"

class HxStr;

namespace Py {

/**
 * Handle on a Python module.
 *
 * `Q22Py6Module` in the RTTI descriptor at `0x008f0930`, with Py::Object at offset 0 as its one
 * base. Its accessor is at `0x005ab958`.
 *
 * The vtable at `0x008257d8`, copied at `0x00833228`, has three entries, and slot 2 repeats
 * Py::Object's own address.
 *
 * | Slot | Member | Address |
 * | ---- | ------ | ------- |
 * | 0 | compiler-generated type function | `0x0050d408` |
 * | 1 | compiler-generated destructor | `0x0050d390` |
 * | 2 | inherited `accepts` | `0x0010eee8` |
 *
 * The inherited slot is the interesting one. Every other narrowing class in the hierarchy
 * overrides accepts(), and this class does not, so a Py::Module accepts any non-null reference.
 * That is surprising rather than wrong, and it is recorded here so that a reader does not mistake
 * the missing override for an omission in the reconstruction.
 */
class Module : public Object {
public:
    /**
     * Import a module by name and wrap it.
     *
     * The interpreter call is `PyImport_ImportModule()` rather than `PyImport_AddModule()`, which
     * released PyCXX uses. Recovered from PyShell's constructor, which imports `__main__` this
     * way, so the body is inlined at that site and has no address of its own.
     *
     * @param name The module name. An empty string arrives at the interpreter as
     *             g_szEmptyString rather than as a null pointer.
     */
    explicit Module(const HxStr &name) : Object() {
        set(PyImport_ImportModule(
            const_cast<char *>(name.mStr != nullptr ? name.mStr : g_szEmptyString)));
        validate();
    }

    /**
     * Produce a handle on the module's namespace.
     *
     * Recovered from PyShell's constructor. The body is inlined at that site and has no address
     * of its own.
     *
     * @return A handle on the module dictionary.
     */
    Dict getDict() const {
        return Dict(PyModule_GetDict(mPtr));
    }
};

} // namespace Py
