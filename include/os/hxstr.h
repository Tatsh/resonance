#pragma once

#include "os/mem.h"

/**
 * Heap-allocated NUL-terminated string.
 *
 * Named after `HxStr.cpp`, the file recorded by its own assert. The member name `mStr` comes from
 * the text of that assert, `mStr != 0`. The class is not polymorphic and has no RTTI. Its
 * destructor is inlined at every call site in the binary, so it is defined here.
 */
class HxStr {
public:
    /**
     * Construct a copy of a C string.
     *
     * A null argument produces an empty string with no allocation.
     *
     * @param pszText The text to copy.
     * @ghidraAddress 0x004b7ad0
     */
    HxStr(const char *pszText);

    /**
     * Construct a copy of another string.
     *
     * @param other The string to copy.
     * @ghidraAddress 0x004b7b50
     */
    HxStr(const HxStr &other);

    /**
     * Replace this string with a copy of another.
     *
     * @param other The string to copy.
     * @return This string.
     * @ghidraAddress 0x004b7e78
     */
    HxStr &operator=(const HxStr &other);

    ~HxStr() {
        if (mStr != nullptr) {
            MemFree(mStr);
        }
    }

    int mLen;   // +0x00 length excluding the terminator
    char *mStr; // +0x04
};
