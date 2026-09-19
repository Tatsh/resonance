#pragma once

#include "script/cxx/config.h"

/**
 * Static registrar that exports one C function to Python.
 *
 * The title is inferred. The class has no RTTI descriptor, no embedded `__FILE__`, and no
 * diagnostic of its own, so nothing in the image establishes what it was called.
 *
 * Every instance is a file-scope static, and the constructor is the whole of the class. Over a
 * hundred translation units create one or more of them in their static initialisation, which is
 * how the game exports its script interface without a central table. `CmdPostScript.cpp` creates
 * two, for `post_script` and `cancel_cmd`.
 *
 * The storage is eight bytes per object and no instruction anywhere writes it, so the class has
 * no recovered state and the constructor's only effect is the registration.
 */
class ScriptFunc {
public:
    /**
     * Add one method to the `hx` module table.
     *
     * The doc string and the calling-convention flag are left at their defaults at every
     * recovered call site, which is an empty doc string and a flag of 1.
     *
     * @param pszName The Python-visible name.
     * @param pfnMethod The C entry point.
     * @ghidraAddress 0x00559720
     */
    ScriptFunc(const char *pszName, PyCFunction pfnMethod);
};
