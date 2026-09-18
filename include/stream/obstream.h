#pragma once

/**
 * Output stream over a caller-supplied byte buffer.
 *
 * `8OBStream` in the RTTI descriptor at `0x0086f5e8`, with no base. Recovery has barely started.
 * The object is 0x20 bytes and has two vptrs, at `+0x00` addressing the table at `0x00824110`
 * and at `+0x04` addressing the table at `0x008240e0` whose entries adjust `this` by `-0x04`. The
 * destructor is slot 8 of the first table rather than slot 1. The buffer occupies `+0x08` and its
 * size `+0x0c`, and the constructor clears `+0x10` through `+0x1f`.
 *
 * The stream writes into memory rather than to a file, so no `FILE` is involved. This declaration
 * exists to satisfy the reference from Globals, which opens one over its log buffer in Init() and
 * destroys it in Shutdown().
 */
class OBStream {
public:
    /**
     * @param pBuffer The buffer to write into.
     * @param nSize The buffer size in bytes.
     * @ghidraAddress 0x004ee2f8
     */
    OBStream(char *pBuffer, int nSize);

    virtual ~OBStream();
};
