#pragma once

#include <stdio.h>

#include "stream/obstream.h"

class HxStr;

/**
 * Output stream over a C library `FILE`.
 *
 * Its RTTI descriptor is at `0x00901bd0`. It has single inheritance from OBStream at offset 0. The
 * type function is at `0x004ed900`. The object is 8 bytes. Its vtable at `0x00824208` runs to slot
 * 6. One call site constructs one.
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
     * @ghidraAddress NTSC-U/C: 0x004edee0
     * @ghidraAddress PAL: 0x0052ca88
     */
    OBFileStream(const HxStr &path);

    /**
     * @ghidraAddress NTSC-U/C: 0x004edf88
     * @ghidraAddress PAL: 0x0052cb30
     */
    virtual OBStream &Write(const void *pSrc, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x004edfc0
     * @ghidraAddress PAL: 0x0052cb68
     */
    virtual OBStream &Reset();

    /**
     * @ghidraAddress NTSC-U/C: 0x004edff0
     * @ghidraAddress PAL: 0x0052cb98
     */
    virtual int Fail();

    /**
     * Close the file.
     *
     * Vtable slot 5, the first virtual this class adds.
     *
     * @ghidraAddress NTSC-U/C: 0x004edf30
     * @ghidraAddress PAL: 0x0052cad8
     */
    virtual ~OBFileStream();

    /**
     * Report the write position.
     *
     * Vtable slot 6, the second virtual this class adds. OBStream has no such slot, which is why
     * this one is new rather than an override.
     *
     * @return The position in bytes from the start of the file.
     * @ghidraAddress NTSC-U/C: 0x004edfc8
     * @ghidraAddress PAL: 0x0052cb70
     */
    virtual int Tell();

private:
    FILE *mFile;
};
