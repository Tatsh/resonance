#pragma once

#include <stdio.h>

#include "os/hxstr.h"
#include "stream/ibstream.h"

/**
 * Input stream over a C library `FILE`.
 *
 * `12IBFileStream` in the RTTI descriptor at `0x00901bc0`, single inheritance from IBStream at
 * offset 0. The type function is at `0x004ed888`. The object is 8 bytes and its vtable is at
 * `0x00824248`, which runs to slot 8 and adds nothing of its own. Two call sites construct one.
 *
 * Every transfer goes to `fread` with a one-byte element size, and the end and failure tests read
 * bits 0x20 and 0x40 of the `FILE` flags rather than calling `feof` and `ferror`, which is the
 * inlined form of those two macros. A path that fails to open produces a null `FILE`, which
 * Fail() then reports, and the destructor closes it with no null test.
 */
class IBFileStream : public IBStream {
public:
    /**
     * Open a file for reading.
     *
     * An empty path arrives at `fopen` as the program-wide empty string rather than as null.
     *
     * @param path The file to open.
     * @ghidraAddress 0x004edd38
     */
    IBFileStream(const HxStr &path);

    /**
     * Close the file.
     *
     * @ghidraAddress 0x004edd88
     */
    virtual ~IBFileStream();

    /** @ghidraAddress 0x004edde0 */
    virtual IBStream &ReadBytes(void *pDest, int nSize);

    /** @ghidraAddress 0x004ede20 */
    virtual IBStream &Seek(int nOffset, int nWhence);

    /** @ghidraAddress 0x004ede78 */
    virtual int Tell();

    /** @ghidraAddress 0x004edea0 */
    virtual int Eof();

    /** @ghidraAddress 0x004edeb8 */
    virtual int Fail();

    /** @ghidraAddress 0x004ede18 */
    virtual IBStream &Flush();

private:
    FILE *mFile;
};
