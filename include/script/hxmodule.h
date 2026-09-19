#pragma once

#include "script/cxx/methodtable.h"

/**
 * Method table of the game's own `hx` extension module.
 *
 * The table is a function-local static, and two routines inline its guard, the ScriptFunc
 * constructor at `0x00559720` and InitHxModule() at `0x005597b8`. Both share one flag at
 * `0x00725dd8` and one object at `0x008de900`, and both register the destructor thunk at
 * `0x00559830` for it, so the accessor is an inline function with no address of its own.
 *
 * Over a hundred static ScriptFunc objects across the image fill the table before the module is
 * registered.
 *
 * @return The shared table.
 */
inline Py::MethodTable &HxMethods() {
    static Py::MethodTable methods;
    return methods;
}

/**
 * Register the `hx` extension module with the interpreter.
 *
 * PyShell's constructor invokes this directly, as its last step, rather than relying on the
 * built-in module table, so `import hx` from a script resolves against a module that is already
 * present.
 *
 * @ghidraAddress 0x005597b8
 */
void InitHxModule();
