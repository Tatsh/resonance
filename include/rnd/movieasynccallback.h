#pragma once

#include "os/asynccallback.h"

/**
 * Receiver of the first read of every movie stream.
 *
 * `18MovieAsyncCallback` in the RTTI, deriving publicly from AsyncCallback at offset 0. One
 * instance at `0x00767940` serves every Rnd::MovieStream. Its destructor at `0x00580708` and its
 * type accessor at `0x00580738` are compiler-generated.
 */
class MovieAsyncCallback : public AsyncCallback {
public:
    /**
     * Hand the completed read to the stream that queued it.
     *
     * The pending list is searched for the stream whose request nHandle is. A failed read, or a
     * header Rnd::MovieStream::ParseHeader() rejects, is fatal. Otherwise the stream is marked
     * loaded. The stream then leaves the list. A handle no stream owns is fatal.
     *
     * @param nHandle The request identifier.
     * @param nFile The file the read was issued against.
     * @param pBuffer The buffer the read filled.
     * @param nLength The bytes read.
     * @param nStatus Zero on success, or a positive failure code.
     * @ghidraAddress 0x00580178
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus);
};
