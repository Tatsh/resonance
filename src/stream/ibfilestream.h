#pragma once

#include <stdio.h>

#include "stream/ibstream.h"

class HxStr;

/**
 * Input stream over a C library `FILE`.
 *
 * Its RTTI descriptor is at `0x00901bc0`. It has single inheritance from IBStream at offset 0. The
 * type function is at `0x004ed888`. The object is 8 bytes. Its vtable at `0x00824248` runs to slot
 * 8 and does not add an entry. Two call sites construct one.
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
     * @ghidraAddress NTSC-U/C: 0x004edd38
     * @ghidraAddress PAL: 0x0052c8e0
     */
    IBFileStream(const HxStr &path);

    /**
     * Close the file.
     *
     * @ghidraAddress NTSC-U/C: 0x004edd88
     * @ghidraAddress PAL: 0x0052c930
     */
    virtual ~IBFileStream();

    /**
     * @ghidraAddress NTSC-U/C: 0x004edde0
     * @ghidraAddress PAL: 0x0052c988
     */
    virtual IBStream &ReadBytes(void *pDest, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x004ede20
     * @ghidraAddress PAL: 0x0052c9c8
     */
    virtual IBStream &Seek(int nOffset, int nWhence);

    /**
     * @ghidraAddress NTSC-U/C: 0x004ede78
     * @ghidraAddress PAL: 0x0052ca20
     */
    virtual int Tell();

    /**
     * @ghidraAddress NTSC-U/C: 0x004edea0
     * @ghidraAddress PAL: 0x0052ca48
     */
    virtual int Eof();

    /**
     * @ghidraAddress NTSC-U/C: 0x004edeb8
     * @ghidraAddress PAL: 0x0052ca60
     */
    virtual int Fail();

    /**
     * @ghidraAddress NTSC-U/C: 0x004ede18
     * @ghidraAddress PAL: 0x0052c9c0
     */
    virtual IBStream &Flush();

private:
    FILE *mFile;
};
