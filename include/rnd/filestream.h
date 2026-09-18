#pragma once

#include <stdio.h>

#include "os/hxstr.h"
#include "rnd/stream.h"

namespace Rnd {

/**
 * Stream over a C library `FILE`.
 *
 * `Q23Rnd10FileStream` in the RTTI descriptor at `0x008eefd8`, single inheritance from
 * `Rnd::Stream` at offset 0. The object is 8 bytes and its vtable is at `0x008261d8`.
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
     * @ghidraAddress 0x0050fe98
     */
    FileStream(const HxStr &path, int nWrite);

    /**
     * Close the file.
     *
     * @ghidraAddress 0x0050ff00
     */
    virtual ~FileStream();

    /** @ghidraAddress 0x0050ff60 */
    virtual Stream &ReadBytes(void *pDest, int nSize);

    /** @ghidraAddress 0x0050ff98 */
    virtual Stream &WriteBytes(const void *pSrc, int nSize);

    /** @ghidraAddress 0x0050ffd0 */
    virtual Stream &Flush();

    /** @ghidraAddress 0x00510000 */
    virtual Stream &Seek(int nOffset, int nWhence);

    /** @ghidraAddress 0x00510058 */
    virtual int Tell();

    /** @ghidraAddress 0x00510080 */
    virtual int Eof();

    /** @ghidraAddress 0x00510098 */
    virtual int Fail();

private:
    FILE *mFile; // +0x04
};

} // namespace Rnd
