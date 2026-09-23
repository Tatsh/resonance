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
 * mStep is 1 while indexes are being read and 2 once the payload read is under way. The payload
 * lands in the shared log stream that Globals::GetResetLog() rewinds, not in mStream.
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

    /**
     * Collect the listed directories and read the first one's index.
     *
     * A failed listing abandons the task, and the body then carries on over the entries all the
     * same. An empty listing reports kMemcardStatusNoFile. Otherwise the first name moves into
     * mCurrentDir and a fresh LoadFileMCT reads `<dir>/index` into mBuffer.
     *
     * @param pOp The finished listing.
     * @ghidraAddress 0x0017c210
     */
    virtual void OnListDir(ListDirOp *pOp);

    /**
     * Search one index for the remix, or finish the payload read.
     *
     * A failed read abandons the task, and the body then carries on all the same. In step 1 the
     * index is parsed from mStream. The first element whose RemixName matches mRemixName starts
     * the payload read of `<dir>/<FileName>` into mPayload and moves to step 2. With no match the
     * next directory's index is read, and with none left the task reports kMemcardStatusNoFile. In
     * step 2 mPayload's size is set from the read and the task reports.
     *
     * @param nStatus The inner read's status.
     * @ghidraAddress 0x0017c650
     */
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

    // The inner read, of an index or of the payload. OnListDir() and OnFileLoaded() delete the
    // previous one before creating the next, and the destructor does not release it. +0x40
    LoadFileMCT *mLoadTask;

    // One index file is read into this stream's buffer at a time. +0x44
    IOBPreallocMemStream mStream;

    // The stream the payload lands in, Globals::GetResetLog() at construction. +0x64
    IOBPreallocMemStream *mPayload;

    // mStream.mBuffer, cached by the constructor. +0x68
    char *mBuffer;
};
