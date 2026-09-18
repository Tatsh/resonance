#pragma once

#include "os/hxstr.h"
#include "rnd/object.h"

/**
 * Request to load one renderer file in the background.
 *
 * Named after the `RndAsyncLoader` allocation tag in its own translation unit. The class is not
 * polymorphic and has no RTTI. A request is constructed with the directory and file to load, then
 * enqueued. Callers advance the queue with PollAsyncLoads() and test the individual request with
 * Poll() until it reports completion. Its members have not been recovered.
 */
class RndAsyncLoader {
public:
    /**
     * Prepare a load request.
     *
     * @param directory The directory to load from.
     * @param file The file to load.
     * @param nPriority The queue priority, or -1 for the default.
     * @ghidraAddress 0x003f7c00
     */
    RndAsyncLoader(const HxStr &directory, const HxStr &file, int nPriority);

    /**
     * Release the request and any partially loaded data.
     *
     * @ghidraAddress 0x003f8178
     */
    ~RndAsyncLoader();

    /**
     * Add this request to the load queue.
     *
     * @ghidraAddress 0x003f8308
     */
    void Enqueue();

    /**
     * Abandon this request.
     *
     * @ghidraAddress 0x003f8030
     */
    void Cancel();

    /**
     * Test whether this request has finished.
     *
     * @param ppLoaded Receives the loaded object.
     * @return Non-zero once the request is complete.
     * @ghidraAddress 0x003f8fc0
     */
    int Poll(Rnd::Object **ppLoaded);

    /**
     * Advance every queued load request.
     *
     * @ghidraAddress 0x003f8930
     */
    static void PollAsyncLoads();
};
