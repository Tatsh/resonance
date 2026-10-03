#pragma once

#include <iostream>

/**
 * Static object through which a source file registers its diagnostic stream with SpewTable.
 *
 * The class has no members and emits no RTTI. Its name comes from the debugging symbols of the
 * North American demo release. The out-of-line constructor has no caller, because each registering
 * unit expands it into its static initialiser.
 */
class SpewRegister {
public:
    /**
     * Register a stream pointer with SpewTable::shared().
     *
     * @param ppStream The file's stream pointer.
     * @param pszFile The source file name.
     * @ghidraAddress NTSC-U/C: 0x004b44c8
     * @ghidraAddress PAL: 0x004f27d8
     */
    SpewRegister(std::ostream **ppStream, const char *pszFile);
};
