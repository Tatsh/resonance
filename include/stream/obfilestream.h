#pragma once

#include <stdio.h>

#include "stream/obstream.h"

class HxStr;

/**
 * Output stream over a C library `FILE`.
 *
 * `12OBFileStream` in the RTTI descriptor at `0x00901bd0`, single inheritance from OBStream at
 * offset 0. The type function is at `0x004ed900`. The object is 8 bytes and its vtable is at
 * `0x00824208`, which runs to slot 6. One call site constructs one.
 *
 * Two slots are added beyond OBStream's four, in the order the declarations below give. The
 * destructor is one of them, because OBStream declares none, so a stream of this class destroyed
 * through an `OBStream *` never closes its file.
 *
 * Every transfer goes to `fwrite` with a one-byte element size, and the failure test reads bit
 * 0x40 of the `FILE` flags rather than calling `ferror`. Reset() does nothing, and in particular
 * does not flush.
 */
class OBFileStream : public OBStream {
public:
    /**
     * Open a file for writing.
     *
     * An empty path arrives at `fopen` as the program-wide empty string rather than as null.
     *
     * @param path The file to open.
     * @ghidraAddress 0x004edee0
     */
    OBFileStream(const HxStr &path);

    /** @ghidraAddress 0x004edf88 */
    virtual OBStream &WriteBytes(const void *pSrc, int nSize);

    /** @ghidraAddress 0x004edfc0 */
    virtual OBStream &Reset();

    /** @ghidraAddress 0x004edff0 */
    virtual int Fail();

    /**
     * Close the file.
     *
     * Vtable slot 5, the first virtual this class adds.
     *
     * @ghidraAddress 0x004edf30
     */
    virtual ~OBFileStream();

    /**
     * Report the write position.
     *
     * Vtable slot 6, the second virtual this class adds. OBStream has no such slot, which is why
     * this one is new rather than an override.
     *
     * @return The position in bytes from the start of the file.
     * @ghidraAddress 0x004edfc8
     */
    virtual int Tell();

private:
    FILE *mFile;
};
