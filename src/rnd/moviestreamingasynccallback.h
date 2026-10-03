#pragma once

#include "os/asynccallback.h"

/**
 * Receiver of the reads that keep a streaming movie's ring buffer full.
 *
 * It derives publicly from AsyncCallback at offset 0. One instance at `0x00767948` serves the one
 * streaming Rnd::AMovieSet the unit tracks. Its destructor at `0x005807b0` and its type accessor
 * at `0x005807e0` are compiler-generated.
 */
class MovieStreamingAsyncCallback : public AsyncCallback {
public:
    /**
     * Commit a completed streaming read to the ring buffer.
     *
     * A failed read is fatal. Otherwise the file offset and the ring buffer's write pointer advance
     * by nLength. Either way another streaming read may then be queued.
     *
     * @param nHandle The request identifier.
     * @param nFile The file the read was issued against.
     * @param pBuffer The buffer the read filled.
     * @param nLength The bytes read.
     * @param nStatus Zero on success, or a positive failure code.
     * @ghidraAddress NTSC-U/C: 0x005809f0
     * @ghidraAddress PAL: 0x005c3a28
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus);
};
