#pragma once

#include <list>

#include "os/hxstr.h"

namespace Rnd {
class Drawable;
class Object;
} // namespace Rnd

/**
 * Request to load one renderer file in the background.
 *
 * The class is not polymorphic and has no RTTI, so its title is inferred. A request is constructed
 * with the directory and file to load, then enqueued. Callers advance the queue with
 * PollAsyncLoads() and test the individual request with Poll() until it reports completion.
 *
 * The object is 0x2c bytes. The constructor builds three `std::list` members at `+0x00`, `+0x04`,
 * and `+0x08`, each of which takes a 0x10-byte sentinel; that bounds their element at 8 bytes or
 * less, so all three are lists of pointers rather than of records. The list at `+0x00` stores the
 * objects the request has produced so far, which is what Poll() counts and what
 * RndAsyncLoader::HarvestLoadedObjects() walks. Everything from `+0x0c` on is declared
 * below. The three flag titles are inferred from their initial values and from the order Poll()
 * tests them in.
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
     * Test whether this request has finished and report how far it has advanced.
     *
     * The fraction is the proportion of the produced objects that have finished loading. A request
     * that failed reports 0 and a request already marked finished reports 1.
     *
     * @param pfProgress Receives a fraction between 0 and 1.
     * @return Non-zero once the request is complete.
     * @ghidraAddress 0x003f8fc0
     */
    int Poll(float *pfProgress);

    /**
     * Advance every queued load request.
     *
     * @ghidraAddress 0x003f8930
     */
    static void PollAsyncLoads();

private:
    // The objects the request has produced so far. Poll() counts it and
    // RndAsyncLoader::HarvestLoadedObjects() walks it.
    std::list<Rnd::Object *> mObjects; // +0x00

public:
    /**
     * Drawables the container load produced.
     *
     * MetScreen::SetShowing() copies the list and shows or hides every entry, which is what fixes
     * the element as a drawable rather than a bare object. Public because that read comes from
     * outside the hierarchy and the image has no accessor to route it through. A friend declaration
     * fits the image equally well. The member is declared between the two private lists so that the
     * recovered order, and therefore the recovered layout, is preserved. +0x04
     */
    std::list<Rnd::Drawable *> mDrawables;

private:
    // The element type is unrecovered. Rnd::Object is the conservative base, and the 0x10-byte
    // sentinel the constructor builds bounds the element at 8 bytes or less either way.
    std::list<Rnd::Object *> mUnknown08; // +0x08
    HxStr mDirectory;                    // +0x0c
    HxStr mFile;                         // +0x14
    // Starts set and reports no progress while it stays set.
    int mPending;  // +0x1c
    int mStarted;  // +0x20
    int mFinished; // +0x24
    int mPriority; // +0x28
};
