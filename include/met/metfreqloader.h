#pragma once

#include <vector>

#include "os/asynccallback.h"
#include "os/hxstr.h"

class MetPersonaData;

/**
 * Asynchronous reader of one pre-fab persona file.
 *
 * `13MetFreqLoader` in the RTTI descriptor at `0x008f2a90`, deriving publicly from AsyncCallback
 * at offset 0. The vtable is at `0x007f7d78`, and the object is 0x1c bytes, which the two
 * allocations in MetFreqMakerAssetManager's constructor fix. The embedded source file name
 * `MetFreqLoader.cpp` is billed with the buffer release in Done().
 *
 * Start() queues a read of the file, Done() parses every persona in the completed buffer into the
 * list the loader was given, and IsLoaded() pumps the asynchronous layer and reports completion.
 *
 * The translation unit spans `0x002a0e30` to `0x002a3820`. Besides the members below, it has
 * template library emissions at `0x002a1090`, `0x002a1268`, `0x002a1638` through `0x002a2a48`,
 * `0x002a2fc0` through `0x002a3190`, `0x002a3638`, `0x002a3700`, and `0x002a37a0`.
 */
class MetFreqLoader : public AsyncCallback {
public:
    /**
     * Record the file to read and the list to fill.
     *
     * @param path The persona file.
     * @param pIdentities The list Done() appends the personas to.
     * @ghidraAddress 0x002a3498
     */
    MetFreqLoader(const HxStr &path, std::vector<MetPersonaData *> *pIdentities);

    /**
     * Release the path.
     *
     * Vtable slot 1.
     *
     * @ghidraAddress 0x002a3430
     */
    virtual ~MetFreqLoader();

    /**
     * Parse the completed read into personas and release the buffer.
     *
     * Vtable slot 2. A negative status changes nothing. Otherwise the buffer is parsed, released
     * under the tag `MetFreqLoader.cpp`, the request handle is cleared, and the loader is marked
     * loaded.
     *
     * @param nHandle The request identifier.
     * @param nFile The file the read was issued against.
     * @param pBuffer The buffer the read filled.
     * @param nLength The bytes read.
     * @param nStatus The completion status.
     * @ghidraAddress 0x002a35d8
     */
    virtual void Done(int nHandle, int nFile, void *pBuffer, int nLength, int nStatus);

    /**
     * Report whether the FreQ maker assets are resident.
     *
     * The body does not read this object. It runs MetFreqMakerAssetManager::PollLoad(). The title
     * is inferred.
     *
     * @return True once the assets are resident.
     * @ghidraAddress 0x002a3500
     */
    bool PollAssets();

    /**
     * Wait for the FreQ maker assets and queue the read of the persona file.
     *
     * The read is issued with no zone selected, and the previously selected zone is restored
     * after. The title is inferred.
     *
     * @ghidraAddress 0x002a3528
     */
    void Start();

    /**
     * Pump the asynchronous layer and report whether Done() has run.
     *
     * The title is inferred.
     *
     * @return Non-zero once the personas are parsed.
     * @ghidraAddress 0x002a35a8
     */
    int IsLoaded();

private:
    // 0x002a0e30. Read a persona count and then each persona from the buffer, marking each one with
    // 1 at MetPersonaData +0x15c, rebuilding its campaign level list, and appending it to
    // mIdentities.
    void ParseIdentities(const void *pBuffer, int nLength);

    std::vector<MetPersonaData *> *mIdentities; // +0x04
    HxStr mPath;                                // +0x08
    int mHandle;                                // +0x10, the queued read, or zero
    int mLoaded;                                // +0x14
    int mStarted;                               // +0x18
};
