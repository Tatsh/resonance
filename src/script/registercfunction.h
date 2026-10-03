#pragma once

#include "script/cxx/config.h"

/**
 * Static registrar that exports one C function to Python.
 *
 * The class has no RTTI descriptor, no embedded `__FILE__`, and no diagnostic of its own. Its name
 * comes from the debugging symbols of the North American demo release. The demo's constructor has
 * the same instructions and parameter types as this class's constructor.
 *
 * Every instance is a file-scope static, and the constructor is the whole of the class. Over a
 * hundred translation units create one or more of them in their static initialisation, which is
 * how the game exports its script interface without a central table. `CmdPostScript.cpp` creates
 * two, for `post_script` and `cancel_cmd`.
 *
 * The storage is eight bytes per object and no instruction anywhere writes it, so the class has
 * no recovered state and the constructor's only effect is the registration.
 */
class RegisterCFunction {
public:
    /**
     * Add one method to the `hx` module table.
     *
     * The doc string and the calling-convention flag are left at their defaults at every
     * recovered call site, which is an empty doc string and a flag of 1.
     *
     * @param pszName The Python-visible name.
     * @param pfnMethod The C entry point.
     * @ghidraAddress NTSC-U/C: 0x00559720
     * @ghidraAddress PAL: 0x0059a878
     */
    RegisterCFunction(const char *pszName, PyCFunction pfnMethod);
};
