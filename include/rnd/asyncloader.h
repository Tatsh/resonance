#pragma once

#include <list>
#include <vector>

#include "os/hxstr.h"
#include "os/mem.h"

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
 * RndAsyncLoader::HarvestLoadedObjects() fills. Everything from `+0x0c` on is declared
 * below. The three flag titles are inferred from their initial values and from the order Poll()
 * tests them in.
 */
class RndAsyncLoader {
public:
    /**
     * Prepare an empty request with no directory or file.
     *
     * The request starts pending, with its file not read and its textures not finished, and with
     * no zone.
     * The image lists no caller for the out-of-line body.
     *
     * @ghidraAddress 0x003f7e50
     */
    RndAsyncLoader();

    /**
     * Prepare a load request.
     *
     * @param directory The directory to load from.
     * @param file The file to load.
     * @param nZone The zone PollAsyncLoads() selects while it reads the objects, or kNoZone.
     * @ghidraAddress 0x003f7c00
     */
    RndAsyncLoader(const HxStr &directory, const HxStr &file, int nZone);

    /**
     * Release the request and any partially loaded data.
     *
     * @ghidraAddress 0x003f8178
     */
    ~RndAsyncLoader();

    /**
     * Add this request to the load queue.
     *
     * A request already in either queue logs an internal error with its directory and file and is
     * not added again. The first request resolves g_nRndLoaderZone from the zone "rndfile". The
     * request is then marked not pending, not read, and not finished, and appended to
     * g_pendingLoads.
     *
     * @ghidraAddress 0x003f8308
     */
    void Enqueue();

    /**
     * Abandon this request.
     *
     * A request whose file has been read is not affected. Otherwise its read is cancelled and its
     * entry removed from g_activeLoads, and it is removed from g_pendingLoads.
     *
     * @ghidraAddress 0x003f8030
     */
    void Cancel();

    /**
     * Abandon this request and point it at another file.
     *
     * Runs Cancel(), replaces the directory and the file, and marks the request pending, not
     * read, and not finished. Unlike Unload() it releases nothing that was already loaded. The
     * image lists no caller, and the title is inferred.
     *
     * @param directory The directory to load from.
     * @param file The file to load.
     * @ghidraAddress 0x003fc708
     */
    void Restart(const HxStr &directory, const HxStr &file);

    /**
     * Abandon this request and release everything it has loaded, leaving it ready to start again.
     *
     * Runs Cancel(), then, unless the request is still pending, deletes every object in mUnknown08
     * through its destructor, clears the three lists, and marks the request pending, not
     * read, and not finished. The destructor runs it first, and Renderer::UnloadCommon() runs it
     * before each delete as well. The title is inferred.
     *
     * @ghidraAddress 0x003f8240
     */
    void Unload();

    /**
     * Allocate a request under the tag `RndAsyncLoader`.
     *
     * Every new-expression open-codes the call.
     *
     * @param nSize The object size the compiler supplies.
     * @return The block.
     */
    static void *operator new(size_t nSize) {
        return AllocateTaggedMemory(nSize, "RndAsyncLoader");
    }

    /**
     * Release a request under the tag `RndAsyncLoader`.
     *
     * The deleting branch of the destructor open-codes the call.
     *
     * @param pBlock The block.
     */
    static void operator delete(void *pBlock) {
        FreeTaggedMemory(pBlock, "RndAsyncLoader");
    }

    /**
     * Test whether this request has finished and report how far it has advanced.
     *
     * The fraction is the proportion of the textures in mObjects whose mip levels have all
     * arrived, each polled once through Rnd::Tex::PollAsyncMips(). A request still pending, or one
     * whose file has not been read, reports 0. A request already marked finished reports 1, and
     * one whose textures are all loaded is marked finished.
     *
     * @param pfProgress Receives a fraction between 0 and 1.
     * @return Non-zero once the request is complete.
     * @ghidraAddress 0x003f8fc0
     */
    int Poll(float *pfProgress);

    /**
     * Advance every queued load request.
     *
     * Runs with g_nRndLoaderZone selected and restores the previous zone on return. The zone is
     * reset when requests are waiting and no read is in flight. Each waiting request then has
     * `gen/<file>.gz` under its directory read into a block of that zone and moves to
     * g_activeLoads, until a file does not fit. A file of length 0 raises
     * "RndAsyncLoader::Poll(): couldn't find: %s".
     *
     * The reads are then collected in issue order. The first one still in flight stops the
     * collection. A failed read logs "ERROR reading RND file async: %s:%s!!" and is dropped. A
     * finished read selects the request's zone, loads the objects through Rnd::g_manager, marks
     * the file read, and ends the collection. Only one file is loaded per call.
     *
     * @ghidraAddress 0x003f8930
     */
    static void PollAsyncLoads();

    /**
     * The objects the request has produced so far. +0x00
     *
     * Poll() counts it and RndAsyncLoader::HarvestLoadedObjects() fills it. Public because
     * MetFreqMakerAssetManager::GetLoadedObjects() and MetFreqMakerAssetManager::PollLoad() copy it
     * directly, and the image has no accessor.
     */
    std::list<Rnd::Object *> mObjects;

    /**
     * Drawables the container load produced.
     *
     * MetScreen::SetShowing() copies the list and shows or hides every entry, which is what fixes
     * the element as a drawable rather than a bare object. Public because that read comes from
     * outside the hierarchy and the image has no accessor to route it through. A friend declaration
     * fits the image equally well. The member is declared between mObjects and the private list
     * after it so that the recovered order, and therefore the recovered layout, is preserved. +0x04
     */
    std::list<Rnd::Drawable *> mDrawables;

private:
    // 0x003f8460. Copy Rnd::g_manager.mLoaded into mUnknown08, then append every `Tex` in mLoaded
    // and then in mMergeObjects to mObjects and every `Text` to mDrawables. PollAsyncLoads() is
    // the one caller, and the name is inferred.
    void HarvestLoadedObjects();

    // Every object the request loaded, copied from Rnd::g_manager.mLoaded.
    std::list<Rnd::Object *> mUnknown08; // +0x08
    HxStr mDirectory;                    // +0x0c
    HxStr mFile;                         // +0x14

public:
    /**
     * Starts set and reports no progress while it stays set. +0x1c
     *
     * Public because MetRenderer's loader routines at `0x00371270` and `0x00371438` read it
     * directly before Enqueue(), and the image has no accessor for it.
     */
    int mPending;

private:
    // Set once PollAsyncLoads() has read the file and loaded its objects.
    int mFileRead; // +0x20
    // Set once Poll() finds every texture in mObjects loaded.
    int mFinished; // +0x24
    // Zone PollAsyncLoads() selects while it loads the objects.
    int mZone; // +0x28
};

/**
 * One file read in flight, 0x10 bytes.
 *
 * The record has no RTTI, and its title is inferred.
 */
struct RndActiveLoadEntry {
    RndAsyncLoader *mRequest; /*!< Request the read belongs to. */
    int mHandle;              /*!< Identifier AsyncLoadFileByPath() returned. */
    void *mBuffer;            /*!< Zone block the file is read into. */
    int mLength;              /*!< Uncompressed length of the file. */
};

/**
 * Zone the file reads are allocated from, kNoZone until the first Enqueue().
 *
 * @ghidraAddress 0x006dba38
 */
extern int g_nRndLoaderZone;

/**
 * Requests waiting for their file read to be issued, in queue order.
 *
 * @ghidraAddress 0x006dba40
 */
extern std::vector<RndAsyncLoader *> g_pendingLoads;

/**
 * File reads issued and not yet collected, in issue order.
 *
 * @ghidraAddress 0x006dba50
 */
extern std::vector<RndActiveLoadEntry> g_activeLoads;
