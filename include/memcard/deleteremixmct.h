#pragma once

#include <vector>

#include "memcard/loadfilemct.h"
#include "memcard/memcardtask.h"
#include "memcard/memcarduser.h"
#include "os/hxstr.h"
#include "stream/iobpreallocmemstream.h"

/**
 * Remove one saved remix from a card and rewrite the index it was listed in.
 *
 * `14DeleteRemixMCT` in the RTTI descriptor at `0x00902370`, with two public non-virtual bases,
 * `MemcardTask` at offset 0 and `MemcardUser` at offset 28. Two vtables belong to the class, the
 * 19-entry primary at `0x007da698` and the 21-entry `MemcardUser` table at `0x007da5e8`, whose
 * first two entries and whose slots 19 and 20 adjust `this` back by 28. Slot 20 needs a thunk here
 * and not in LoadRemixMCT, because this class overrides it. An instance is 0x7c bytes, from the
 * constructor's own highest store at `+0x78`. Nothing derives from the class, so that is a lower
 * bound.
 *
 * The task enquires about the card, lists `/BASCUS-97125r*`, reads each index to find the remix,
 * deletes the payload file, and then saves the shortened index. It owns two inner tasks, a
 * `LoadFileMCT` at `+0x50` and a `SaveFileMCT` at `+0x54`, and receives both reports as a
 * `MemcardUser`, which is why it derives from both interfaces and is the only one of the four
 * remix tasks that overrides OnFileSaved().
 *
 * Three routines are recovered and not written. `OnListDir()` at `0x0017d248` collects the listed
 * directory names, `OnFileLoaded()` at `0x0017d690` parses one index, and `DeleteNextFile()` at
 * `0x0017dc68` drives the deletion and the index rewrite. All three walk the `RemixIndex` record
 * whose class cannot be titled from the image. The constructor at `0x0017ce08` is written, because
 * every field it touches has a recovered type.
 *
 * The method titles ListRemixDir(), DeleteNextFile(), Execute(), and Finish() are inferred. No
 * string in the image identifies any of them.
 */
class DeleteRemixMCT : public MemcardTask, public MemcardUser {
public:
    /**
     * Construct an idle deletion.
     *
     * mStatus and mStep are not written, so a task that is read before it reports exposes whatever
     * those two words stored when the block was allocated.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param pCookie The tag that abandons exactly this task's operations.
     * @param remixName The remix to remove.
     * @ghidraAddress 0x0017ce08
     */
    DeleteRemixMCT(
        MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie, const HxStr &remixName);

    /** @ghidraAddress 0x001854e0 */
    virtual ~DeleteRemixMCT();

    /**
     * List every remix save directory on the card.
     *
     * Clears mStep first.
     *
     * @ghidraAddress 0x0017d098
     */
    void ListRemixDir();

    /** @ghidraAddress 0x0017dc68 */
    void DeleteNextFile();

    /**
     * Start the listing unless the card reported that it cannot be read.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress 0x00186cc0
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Advance the deletion, abandoning the task first when the deletion failed.
     *
     * The next step runs whether the deletion succeeded or not, because the abandon path is taken
     * before the step call rather than instead of it.
     *
     * @param pOp The finished deletion.
     * @ghidraAddress 0x00186d40
     */
    virtual void OnDeleteFile(DeleteFileOp *pOp);

    /**
     * Report the finished deletion through MemcardUser::OnRemixDeleted().
     *
     * @ghidraAddress 0x00186db8
     */
    virtual void Finish();

    /**
     * Enquire about the card and start the deletion.
     *
     * @ghidraAddress 0x00186c90
     */
    virtual void Execute();

    /** @ghidraAddress 0x0017d690 */
    virtual void OnFileLoaded(int nStatus);

    /**
     * Record the status of the index rewrite and advance to the next step.
     *
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress 0x00186d98
     */
    virtual void OnFileSaved(int nStatus);

private:
    // Selects the step the three step bodies run next. ListRemixDir() clears it and the
    // constructor does not write it. +0x20
    int mStep;

    // The directory the index is being read out of. +0x24
    HxStr mCurrentDir;

    // Every remix save directory the listing found, consumed one per step. +0x2c
    std::vector<HxStr> mDirNames;

    // +0x38
    HxStr mUnknown38;

    // The remix to remove, as the constructor received it. +0x40
    HxStr mRemixName;

    // +0x48
    HxStr mUnknown48;

    // The inner read task. Recorded as a reserved word rather than as a pointer, because the
    // destructor releases it through a virtual slot and no step body that would fix its class is
    // written. +0x50
    unsigned char mReserved50[4];

    // The inner save task that rewrites the index, recorded for the same reason. +0x54
    unsigned char mReserved54[4];

    // One index or payload file passes through this stream's buffer at a time. +0x58
    IOBPreallocMemStream mStream;

    // mStream.mBuffer, cached by the constructor. +0x78
    char *mBuffer;
};
