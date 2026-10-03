#pragma once

#include <vector>

#include "memcard/loadfilemct.h"
#include "memcard/memcardtask.h"
#include "memcard/memcarduser.h"
#include "memcard/savefilemct.h"
#include "os/hxstr.h"
#include "stream/iobpreallocmemstream.h"

/**
 * Remove one saved remix from a card and rewrite the index it was listed in.
 *
 * Its RTTI descriptor is at `0x00902370`. It has two public non-virtual bases, `MemcardTask` at
 * offset 0 and `MemcardUser` at offset 28. Two vtables belong to the class, the 19-entry primary at
 * `0x007da698` and the 21-entry `MemcardUser` table at `0x007da5e8`, whose first two entries and
 * whose slots 19 and 20 adjust `this` back by 28. Slot 20 needs a thunk here and not in
 * LoadRemixMCT, because this class overrides it. An instance is 0x7c bytes, from the constructor's
 * highest store at `+0x78`. No class derives from it. The size 0x7c is therefore a lower
 * bound.
 *
 * The task enquires about the card, lists `/BASCUS-97125r*`, reads each index to find the remix,
 * deletes the payload file, and then saves the shortened index. It has two inner tasks, a
 * `LoadFileMCT` at `+0x50` and a `SaveFileMCT` at `+0x54`, and receives both reports as a
 * `MemcardUser`. It therefore derives from both interfaces and is the only one of the four remix
 * tasks that overrides OnFileSaved().
 *
 * mStep is 1 while indexes are read, 2 once the remix is found, 3 once the shortened index is
 * saved, and 4 once the payload file is deleted.
 *
 * The method titles ListRemixDir(), DeleteNextFile(), Execute(), and Finish() are inferred. No
 * string in the image identifies any of them.
 */
class DeleteRemixMCT : public MemcardTask, public MemcardUser {
public:
    /**
     * Construct an idle deletion.
     *
     * mStatus and mStep are not written. A task that is read before it reports therefore exposes
     * whatever those two words stored when the block was allocated.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param remixName The remix to remove.
     * @ghidraAddress NTSC-U/C: 0x0017ce08
     * @ghidraAddress PAL: 0x001814d0
     */
    DeleteRemixMCT(
        MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, const HxStr &remixName);

    /**
     * @ghidraAddress NTSC-U/C: 0x001854e0
     * @ghidraAddress PAL: 0x0018ae48
     */
    virtual ~DeleteRemixMCT();

    /**
     * List every remix save directory on the card.
     *
     * Clears mStep first.
     *
     * @ghidraAddress NTSC-U/C: 0x0017d098
     * @ghidraAddress PAL: 0x001817b0
     */
    void ListRemixDir();

    /**
     * Run the deletion step mStep selects.
     *
     * A failed step reports at once. In step 2 the index in mStream is parsed, the element whose
     * FileName matches mFileName is erased (a missing one reports kMemcardStatusNoFile), the
     * shortened index is written back into mStream, and a fresh SaveFileMCT saves it as
     * `<dir>/index` under g_remixIconTitle plus the directory number, moving to step 3. In step 3
     * the payload `<dir>/<mFileName>` is deleted, moving to step 4. Step 4 reports. The European
     * release gives the save the file name `index`, because its SaveFileMCT supplies the `/`.
     *
     * @ghidraAddress NTSC-U/C: 0x0017dc68
     * @ghidraAddress PAL: 0x00182568
     */
    void DeleteNextFile();

    /**
     * Collect the listed directories and read the first one's index.
     *
     * The body is LoadRemixMCT::OnListDir()'s, except that mStream is rewound before the read.
     *
     * @param pOp The finished listing.
     * @ghidraAddress NTSC-U/C: 0x0017d248
     * @ghidraAddress PAL: 0x001819d8
     */
    virtual void OnListDir(ListDirOp *pOp);

    /**
     * Start the listing unless the card reported that it cannot be read.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress NTSC-U/C: 0x00186cc0
     * @ghidraAddress PAL: 0x0018c498
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Advance the deletion, abandoning the task first when the deletion failed.
     *
     * The next step runs whether the deletion succeeded or not, because the abandon path is taken
     * before the step call rather than instead of it.
     *
     * @param pOp The finished deletion.
     * @ghidraAddress NTSC-U/C: 0x00186d40
     * @ghidraAddress PAL: 0x0018c518
     */
    virtual void OnDeleteFile(DeleteFileOp *pOp);

    /**
     * Report the finished deletion through MemcardUser::OnRemixDeleted().
     *
     * @ghidraAddress NTSC-U/C: 0x00186db8
     * @ghidraAddress PAL: 0x0018c590
     */
    virtual void Finish();

    /**
     * Enquire about the card and start the deletion.
     *
     * @ghidraAddress NTSC-U/C: 0x00186c90
     * @ghidraAddress PAL: 0x0018c468
     */
    virtual void Execute();

    /**
     * Search one index for the remix.
     *
     * A failed read abandons the task, and the body then continues all the same. mStep becomes 1
     * and the index is parsed from mStream. The first element whose RemixName matches mRemixName
     * records its FileName in mFileName, rewinds mStream for DeleteNextFile(), and moves to step 2.
     * With no match the next directory's index is read, and with none left the task reports
     * kMemcardStatusNoFile.
     *
     * @param nStatus The inner read's status.
     * @ghidraAddress NTSC-U/C: 0x0017d690
     * @ghidraAddress PAL: 0x00181ef8
     */
    virtual void OnFileLoaded(int nStatus);

    /**
     * Record the status of the index rewrite and advance to the next step.
     *
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress NTSC-U/C: 0x00186d98
     * @ghidraAddress PAL: 0x0018c570
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

    // The payload file of the remix, copied from its index element by OnFileLoaded(). +0x38
    HxStr mFileName;

    // The remix to remove, as the constructor received it. +0x40
    HxStr mRemixName;

    // Constructed empty and destroyed, and never otherwise read or written. +0x48
    HxStr mUnusedString;

    // The inner read of each index. The destructor deletes it. +0x50
    LoadFileMCT *mLoadTask;

    // The inner save that rewrites the shortened index. The destructor deletes it. +0x54
    SaveFileMCT *mSaveTask;

    // One index or payload file passes through this stream's buffer at a time. +0x58
    IOBPreallocMemStream mStream;

    // mStream.mBuffer, cached by the constructor. +0x78
    char *mBuffer;
};
