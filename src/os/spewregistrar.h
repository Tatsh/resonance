#pragma once

#include <iostream>

/**
 * Static object through which a source file registers its diagnostic stream with Spew.
 *
 * The class has no members and emits no RTTI, so the name is inferred from what its constructor
 * does. The out-of-line constructor has no caller, because each registering unit expands it into
 * its static initialiser.
 */
class SpewRegistrar {
public:
    /**
     * Register a stream pointer with Spew::shared().
     *
     * @param ppStream The file's stream pointer.
     * @param pszFile The source file name.
     * @ghidraAddress 0x004b44c8
     */
    SpewRegistrar(std::ostream **ppStream, const char *pszFile);
};
