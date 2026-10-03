#pragma once

#include <stdio.h>

#include "rnd/stream.h"

class HxStr;

namespace Rnd {

/**
 * Stream over a C library `FILE`.
 *
 * Its RTTI descriptor is at `0x008eefd8`. It has single inheritance from `Rnd::Stream` at offset 0.
 * The object is 8 bytes and its vtable is at `0x008261d8`.
 *
 * Every transfer goes straight to `fread` or `fwrite` with a one-byte element size, and the end
 * and failure tests read bits 0x20 and 0x40 of the `FILE` flags rather than calling `feof` and
 * `ferror`. A path that fails to open leaves the `FILE` null, which Fail() then reports.
 */
class FileStream : public Stream {
public:
    /**
     * Open a file.
     *
     * An empty path arrives at `fopen` as the program-wide empty string rather than as null.
     *
     * @param path The file to open.
     * @param nWrite Non-zero to open for writing, which uses mode "wb" rather than "rb".
     * @ghidraAddress NTSC-U/C: 0x0050fe98
     * @ghidraAddress PAL: 0x0054f480
     */
    FileStream(const HxStr &path, int nWrite);

    /**
     * Close the file.
     *
     * @ghidraAddress NTSC-U/C: 0x0050ff00
     * @ghidraAddress PAL: 0x0054f4e8
     */
    virtual ~FileStream();

    /**
     * @ghidraAddress NTSC-U/C: 0x0050ff60
     * @ghidraAddress PAL: 0x0054f548
     */
    virtual Stream &ReadBytes(void *pDest, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x0050ff98
     * @ghidraAddress PAL: 0x0054f580
     */
    virtual Stream &WriteBytes(const void *pSrc, int nSize);

    /**
     * @ghidraAddress NTSC-U/C: 0x0050ffd0
     * @ghidraAddress PAL: 0x0054f5b8
     */
    virtual Stream &Flush();

    /**
     * @ghidraAddress NTSC-U/C: 0x00510000
     * @ghidraAddress PAL: 0x0054f5e8
     */
    virtual Stream &Seek(int nOffset, int nWhence);

    /**
     * @ghidraAddress NTSC-U/C: 0x00510058
     * @ghidraAddress PAL: 0x0054f640
     */
    virtual int Tell();

    /**
     * @ghidraAddress NTSC-U/C: 0x00510080
     * @ghidraAddress PAL: 0x0054f668
     */
    virtual int Eof();

    /**
     * @ghidraAddress NTSC-U/C: 0x00510098
     * @ghidraAddress PAL: 0x0054f680
     */
    virtual int Fail();

private:
    FILE *mFile; // +0x04
};

} // namespace Rnd
