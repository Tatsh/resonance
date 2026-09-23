#pragma once

#include <vector>

#include "memcard/loadfilemct.h"
#include "memcard/memcardtask.h"
#include "memcard/memcarduser.h"
#include "os/hxstr.h"
#include "stream/iobpreallocmemstream.h"

/**
 * Read one saved remix off a card.
 *
 * `12LoadRemixMCT` in the RTTI descriptor at `0x008ef048`, with two public non-virtual bases,
 * `MemcardTask` at offset 0 and `MemcardUser` at offset 28. Two vtables belong to the class, the
 * 19-entry primary at `0x007da7e8` and the 21-entry `MemcardUser` table at `0x007da738`. Only the
 * first two entries and slot 19 of the second table adjust `this`; slot 20 is the inherited
 * `MemcardUser::OnFileSaved()` body and therefore needs no thunk. An instance is 0x6c bytes, from
 * the constructor's own highest store at `+0x68`. Nothing derives from the class, so that is a
 * lower bound rather than a settled size.
 *
 * The task enquires about the card, lists `/BASCUS-97125r*`, and then reads the index out of each
 * remix save directory through a `LoadFileMCT` of its own until it finds the requested remix. It
 * receives that inner task's report as a `MemcardUser`, which is why it derives from both
 * interfaces.
 *
 * Three routines are recovered and not written. `OnListDir()` at `0x0017c210` collects the listed
 * directory names, and `OnFileLoaded()` at `0x0017c650` parses one index and either reads the
 * remix payload or steps to the next directory. Both walk the `RemixIndex` record whose class
 * cannot be titled from the image. The constructor at `0x0017be28` is not written, because the
 * field at `+0x64` it fills from the application object has no recovered type.
 *
 * The method titles ListRemixDir(), Execute(), and Finish() are inferred. No string in the image
 * identifies any of them.
 */
class LoadRemixMCT : public MemcardTask, public MemcardUser {
public:
    /**
     * Construct an idle load of one remix.
     *
     * MemcardManager::CreateLoadRemixTask() at `0x001f36c8` is the one caller. Not written, for
     * the reason recorded in the class documentation.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param remixName The remix to read, copied into mRemixName.
     * @ghidraAddress 0x0017be28
     */
    LoadRemixMCT(
        MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, const HxStr &remixName);

    /** @ghidraAddress 0x00185380 */
    virtual ~LoadRemixMCT();

    /**
     * List every remix save directory on the card.
     *
     * Clears mStep first, so the listing always restarts the walk from its first step.
     *
     * @ghidraAddress 0x0017c060
     */
    void ListRemixDir();

    /**
     * Start the listing unless the card reported that it cannot be read.
     *
     * Unlike SaveRemixMCT, the enquiry does not test the free space, because a load writes
     * nothing.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress 0x00186bd0
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Report the finished load through MemcardUser::OnRemixLoaded().
     *
     * @ghidraAddress 0x00186c50
     */
    virtual void Finish();

    /**
     * Enquire about the card and start the load.
     *
     * @ghidraAddress 0x00186ba0
     */
    virtual void Execute();

    /** @ghidraAddress 0x0017c650 */
    virtual void OnFileLoaded(int nStatus);

private:
    // Selects the step OnListDir() and OnFileLoaded() run next. ListRemixDir() clears it and the
    // constructor does not write it. +0x20
    int mStep;

    // The remix to read, as the constructor received it. +0x24
    HxStr mRemixName;

    // The directory the index is being read out of. +0x2c
    HxStr mCurrentDir;

    // Every remix save directory the listing found, consumed one per step. +0x34
    std::vector<HxStr> mDirNames;

    // The inner read task, created and released by the two step bodies. Recorded as a reserved
    // word rather than as a pointer, because the destructor does not release it and no step body
    // that would fix its type is written. +0x40
    unsigned char mReserved40[4];

    // One index or payload file is read into this stream's buffer at a time. +0x44
    IOBPreallocMemStream mStream;

    // Filled from the application object at `0x00118f08`, whose type is not recovered. +0x64
    unsigned char mReserved64[4];

    // mStream.mBuffer, cached by the constructor. +0x68
    char *mBuffer;
};
