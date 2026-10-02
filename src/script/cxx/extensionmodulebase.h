#pragma once

#include "os/hxstr.h"
#include "script/cxx/methodtable.h"

namespace Py {

/**
 * Base of an extension module the binding registers with the interpreter.
 *
 * `Q22Py19ExtensionModuleBase` in the RTTI descriptor at `0x0086f650`, with no base. Its accessor
 * is at `0x005aba10`.
 *
 * The object is 0x20 bytes, the module name at `+0x00`, the method table at `+0x08`, and the vptr
 * after both at `+0x1c`. The vtable at `0x008331f8` has two entries, the type function and the
 * destructor. Released PyCXX also stores the qualified module name and declares the method
 * invokers as pure virtual members, and neither is present in the image.
 *
 * The game does not use the class for its `hx` module, because InitHxModule() at `0x005597b8`
 * drives `Py_InitModule4()` directly from a Py::MethodTable rather than through a module object.
 * No routine of the image constructs one.
 */
class ExtensionModuleBase {
public:
    /**
     * Record the module name and start an empty method table.
     *
     * @param pszName The module name.
     * @ghidraAddress 0x005a5d28
     */
    explicit ExtensionModuleBase(const char *pszName);

    /**
     * Release the method table and the name.
     *
     * @ghidraAddress 0x005a5d98
     */
    virtual ~ExtensionModuleBase();

protected:
    /**
     * Register the module with the interpreter.
     *
     * The module's `self` is a new Py::ExtensionModuleBasePtr pointing back at this object.
     *
     * @param pszModuleDoc The module doc string.
     * @ghidraAddress 0x005a5ea0
     */
    void initialize(const char *pszModuleDoc);

    /** The module name. */
    const HxStr mModuleName;

    /** The methods to register. */
    MethodTable mMethodTable;
};

} // namespace Py
