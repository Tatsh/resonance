#pragma once

#include <vector>

#include "game/freqappearance.h"
#include "memcard/loadfilemct.h"
#include "memcard/memcardtask.h"
#include "memcard/memcarduser.h"
#include "memcard/remixdirinfo.h"
#include "memcard/savefilemct.h"
#include "os/hxstr.h"
#include "stream/iobpreallocmemstream.h"

class ListDirOp;

#ifndef VIDEO_STANDARD_PAL
/** Free clusters a remix save needs, as the card enquiry reports them in CheckInfoOp::mFree. */
constexpr int kRemixSaveMinimumFreeClusters = 60;
#endif

/**
 * Write one remix to a card and add it to the index of the directory it lands in.
 *
 * Its RTTI descriptor is at `0x008ef5e0`. It has two public non-virtual bases, `MemcardTask` at
 * offset 0 and `MemcardUser` at offset 28. Two vtables belong to the class, the 19-entry primary at
 * `0x007da938` and the 21-entry `MemcardUser` table at `0x007da888`, whose first two entries and
 * whose slots 19 and 20 adjust `this` back by 28. An instance is 0xa4 bytes, from the constructor's
 * highest store at `+0xa0`. No class derives from it. The size 0xa4 is therefore a lower
 * bound rather than a settled size.
 *
 * The task enquires about the card, rejects a card with fewer than kRemixSaveMinimumFreeClusters
 * free clusters, lists `/BASCUS-97125r*`, and then reads the index out of each remix save directory
 * (step 1), recording a RemixDirInfo for each. A directory whose index already lists a remix of
 * the same name becomes the target and the save replaces that entry. Otherwise ChooseTargetDir()
 * picks the first directory with room, or a fresh one. The task then reads the target's index
 * (step 2), writes the payload from the shared log stream (step 3), and finally rewrites the
 * target's index (step 4). It has a LoadFileMCT and a SaveFileMCT, and receives both reports as a
 * `MemcardUser`. It therefore derives from both interfaces.
 *
 * The European release derives the class from `SaveFileMCT` at offset 0 instead of `MemcardTask`,
 * with `MemcardUser` at offset 0x41c. The primary vtable is at `0x0081e7a8`, the `MemcardUser`
 * table at `0x0081e6f8`, and an instance is 0x4a0 bytes, from
 * MemcardManager::CreateSaveRemixTask()'s allocation. Execute() lists the remix directories at
 * once, without the card enquiry and its free-cluster test. Once the target's index is read,
 * WriteIndex() hands the payload and the rewritten index to the inherited save sequence together.
 * That sequence checks the space both files need. The task has an inner LoadFileMCT and no inner
 * SaveFileMCT.
 *
 * The method titles are inferred. No string in the image identifies any of them.
 */
#ifdef VIDEO_STANDARD_PAL
class SaveRemixMCT : public SaveFileMCT, public MemcardUser {
#else
class SaveRemixMCT : public MemcardTask, public MemcardUser {
#endif
public:
    /**
     * Construct an idle remix save.
     *
     * MemcardManager::CreateSaveRemixTask() at `0x001f31c8` is the one caller. mStep and
     * mTargetIndexStatus are not written.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param remixName The remix's name, copied into mRemixName.
     * @param appearances The players' appearances, copied into mAppearances.
     * @param levelName The level the remix was built over, copied into mLevelName.
     * @param nAlbumNum The album number the index entry records.
     * @ghidraAddress NTSC-U/C: 0x00179ec0
     * @ghidraAddress PAL: 0x0017e068
     */
    SaveRemixMCT(MemcardUser *pUser,
                 Memcard *pCard,
                 int nPortSlot,
                 int nCookie,
                 const HxStr &remixName,
                 const std::vector<FreqAppearance> &appearances,
                 const HxStr &levelName,
                 int nAlbumNum);

    /**
     * Delete the inner save and the inner read.
     *
     * The European release has no inner save.
     *
     * @ghidraAddress NTSC-U/C: 0x001850a8
     * @ghidraAddress PAL: 0x0018a898
     */
    virtual ~SaveRemixMCT();

    /**
     * List every remix save directory on the card.
     *
     * Unlike the other three remix tasks, this one does not clear mStep, because Execute() has
     * already cleared it.
     *
     * @ghidraAddress NTSC-U/C: 0x0017a778
     * @ghidraAddress PAL: 0x0017eb60
     */
    void ListRemixDir();

    /**
     * Rewrite the target directory's index with the saved remix, and save it.
     *
     * The index read in step 2 is parsed again from the start of mStream, or started afresh at
     * version 1 when that read failed. When mReplacing is set, the entry with the same RemixName
     * is refreshed in place. Otherwise a new entry is appended. mStream then receives the
     * rewritten index, and a fresh SaveFileMCT saves it with its icon files as `<dir>/index`.
     *
     * The European release first sets mState to kMemcardTaskRunning and adds the payload from the
     * shared log stream to SaveFileMCT::mFiles. It then adds the rewritten index as `index` and
     * runs SaveFileMCT::Execute() with mTargetDir as the directory.
     *
     * @ghidraAddress NTSC-U/C: 0x0017b318
     * @ghidraAddress PAL: 0x0017f868
     */
    void WriteIndex();

#ifndef VIDEO_STANDARD_PAL
    /**
     * Start the listing unless the card cannot be read or has too little room.
     *
     * A card with fewer than kRemixSaveMinimumFreeClusters free clusters reports
     * kMemcardStatusCardFull and abandons the task rather than proceeding to the listing. The
     * European release inherits SaveFileMCT::OnCheckInfo() instead.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress NTSC-U/C: 0x00186a40
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);
#endif

    /**
     * Collect the listed directories and start the index walk.
     *
     * A failed listing collects nothing. With directories listed the first index is read in step
     * 1. With none, the target and payload file name are chosen at once and step 2 starts.
     *
     * @param pOp The finished listing.
     * @ghidraAddress NTSC-U/C: 0x0017a430
     * @ghidraAddress PAL: 0x0017e768
     */
    virtual void OnListDir(ListDirOp *pOp);

    /**
     * Report the finished save through MemcardUser::OnRemixSaved().
     *
     * @ghidraAddress NTSC-U/C: 0x00186b60
     * @ghidraAddress PAL: 0x0018c338
     */
    virtual void Finish();

    /**
     * Enquire about the card and start the save.
     *
     * The European release runs ListRemixDir() instead of the enquiry.
     *
     * @ghidraAddress NTSC-U/C: 0x00186a08
     * @ghidraAddress PAL: 0x0018c310
     */
    virtual void Execute();

    /**
     * Advance the index walk, or move from the target's index to the payload write.
     *
     * A failure outside step 2 abandons the task. In step 1 the index is parsed, recorded through
     * AppendDirInfo(), and each element's file number raises the directory's highestFileNumber.
     * An element named mRemixName makes this directory the target and its file name the payload
     * file name, and step 2 starts. Otherwise the next directory is read, or the target and payload
     * file name are chosen and step 2 starts. In step 2 the read status, success or not, is
     * recorded in mTargetIndexStatus and step 3 writes the payload. In the European release step 3
     * runs WriteIndex() at once.
     *
     * @param nStatus The inner read's status.
     * @ghidraAddress NTSC-U/C: 0x0017ad70
     * @ghidraAddress PAL: 0x0017f260
     */
    virtual void OnFileLoaded(int nStatus);

#ifndef VIDEO_STANDARD_PAL
    /**
     * Advance past a finished write, or rewrite the index once the payload has been written.
     *
     * The status is recorded whether the write succeeded or not. A successful write at step 3 moves
     * to step 4 and rewrites the index, and every other successful write reports the task
     * finished.
     *
     * @param nStatus One of MemcardStatus.
     * @ghidraAddress NTSC-U/C: 0x00186ad8
     */
    virtual void OnFileSaved(int nStatus);
#endif

private:
    // NTSC-U/C: 0x00179c28, PAL: 0x0017dd60
    // Appends a summary of one directory to infos, with the directory number parsed out of name
    // and no highest file number yet.
    static void AppendDirInfo(std::vector<RemixDirInfo> &infos, const HxStr &name, int nEntryCount);

    // NTSC-U/C: 0x00179d60, PAL: 0x0017ded0
    // Returns the first directory with room, or a fresh name one past the highest directory number.
    static HxStr ChooseTargetDir(const std::vector<RemixDirInfo> &infos);

    // NTSC-U/C: 0x0017a928, PAL: 0x0017ed88
    // Moves the first entry of mDirNames into mCurrentDir, erases it, rewinds mStream, and reads
    // `<dir>/index` through a fresh inner LoadFileMCT.
    void ReadNextIndex();

    // NTSC-U/C: 0x0017ab50, PAL: 0x0017f000
    // Writes mRemixName into the payload in the shared log stream, rewinds mStream, and reads the
    // target directory's index through a fresh inner LoadFileMCT.
    void ReadTargetIndex();

#ifndef VIDEO_STANDARD_PAL
    // NTSC-U/C: 0x0017ba98
    // Saves the payload in the shared log stream as `<target>/<mPayloadFileName>`, without icon
    // files, through a fresh inner SaveFileMCT.
    void WritePayload();
#endif

    // The European release places every member from here on 0x400 bytes further in.

    // Selects the step the bodies run next. Execute() clears it. In the European release it hides
    // SaveFileMCT::mStep. +0x20
    int mStep;

    // The status of the step-2 read of the target's index, tested by WriteIndex(). +0x24
    int mTargetIndexStatus;

    // The directory the index is being read out of. +0x28
    HxStr mCurrentDir;

    // Every remix save directory the listing found, consumed one per step. +0x30
    std::vector<HxStr> mDirNames;

    // A summary of every directory the listing found, built one entry per directory. +0x3c
    std::vector<RemixDirInfo> mDirInfos;

    // The directory the remix lands in. +0x48
    HxStr mTargetDir;

    // The payload's file name inside mTargetDir. +0x50
    HxStr mPayloadFileName;

    // The album number the index entry records. +0x58
    int mAlbumNum;

    // Non-zero once an entry named mRemixName was found. The save then replaces that entry. +0x5c
    int mReplacing;

    // The remix's name. +0x60
    HxStr mRemixName;

    // The players' appearances the index entry records. +0x68
    std::vector<FreqAppearance> mAppearances;

    // The level the remix was built over. +0x74
    HxStr mLevelName;

    // One index file passes through this stream's buffer at a time. +0x7c
    IOBPreallocMemStream mStream;

    // The inner read. The destructor deletes it. +0x9c
    LoadFileMCT *mLoadTask;

#ifndef VIDEO_STANDARD_PAL
    // The inner save. The destructor deletes it. +0xa0
    SaveFileMCT *mSaveTask;
#endif
};
