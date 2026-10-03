#pragma once

#include "script/cxx/extensionmodulebase.h"
#include "script/cxx/pythonextension.h"

namespace Py {

/**
 * Python-visible wrapper that points back at an extension module's C++ object.
 *
 * Its RTTI descriptor is at `0x00902360`. It has `Py::PythonExtension<Py::ExtensionModuleBasePtr>`
 * at offset 0 as its one base. Its accessor is at `0x005abdb8`, and the mangled name string sits at
 * `0x00833488`.
 *
 * The class passes itself as its base's template parameter, which is how PyCXX gives an extension
 * type its own Python type object. Released PyCXX hides the same wrapper inside `Extensions.hxx`
 * so that a module method can recover the C++ module object from the `self` argument.
 *
 * The object is 0x10 bytes, the module pointer following the base. The vtable at `0x00832d50`
 * matches the base's apart from the type function and the destructor.
 */
class ExtensionModuleBasePtr : public PythonExtension<ExtensionModuleBasePtr> {
public:
    /**
     * Wrap a module.
     *
     * Inline. ExtensionModuleBase::initialize() at `0x005a5ea0` expands it.
     *
     * @param pModule The module.
     */
    explicit ExtensionModuleBasePtr(ExtensionModuleBase *pModule) : mModule(pModule) {
    }

    /**
     * Destroy the wrapper without destroying the module.
     *
     * @ghidraAddress NTSC-U/C: 0x005abf30
     * @ghidraAddress PAL: 0x005ee458
     */
    virtual ~ExtensionModuleBasePtr() {
    }

    /** The wrapped module. */
    ExtensionModuleBase *mModule;
};

} // namespace Py
