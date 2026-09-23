#pragma once

#include <vector>

#include "memcard/loadfilemct.h"
#include "memcard/memcardtask.h"
#include "memcard/memcarduser.h"
#include "os/hxstr.h"
#include "stream/iobpreallocmemstream.h"

struct MetRemixRecord;

/**
 * Enumerate every remix saved on one card.
 *
 * `14ListRemixesMCT` in the RTTI descriptor at `0x00902a10`, with two public non-virtual bases,
 * `MemcardTask` at offset 0 and `MemcardUser` at offset 28. Two vtables belong to the class, the
 * 19-entry primary at `0x007da548` and the 21-entry `MemcardUser` table at `0x007da498`, whose
 * first two and whose slot 19 entries adjust `this` back by 28. An instance is 0x64 bytes, which
 * the constructor's own highest store at `+0x60` bounds from below; nothing derives from the class,
 * so no derived first member pins the size from outside and 0x64 is a lower bound rather than a
 * settled size.
 *
 * The task enquires about the card, lists `/BASCUS-97125r*` to find every remix save directory, and
 * then reads the index file out of each one through a `LoadFileMCT` of its own, receiving that
 * inner task's report as a `MemcardUser`. That arrangement is why the class derives from both
 * interfaces.
 *
 * Each index is parsed as a RemixIndex, and every RemixIndexElement becomes one MetRemixRecord
 * appended to mRecords. The element's Version is not carried over.
 *
 * The method titles ListRemixDir(), Execute(), and Finish() are inferred. No string in the image
 * identifies any of them.
 */
class ListRemixesMCT : public MemcardTask, public MemcardUser {
public:
    /**
     * Construct an idle listing.
     *
     * MemcardManager::CreateListRemixesTask() at `0x001f3598` is the one caller.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param pRecords The collection the parse fills, stored in mRecords.
     * @ghidraAddress 0x0017e528
     */
    ListRemixesMCT(MemcardUser *pUser,
                   Memcard *pCard,
                   int nPortSlot,
                   int nCookie,
                   std::vector<MetRemixRecord> *pRecords);

    /** @ghidraAddress 0x001856a8 */
    virtual ~ListRemixesMCT();

    /**
     * List every remix save directory on the card.
     *
     * The pattern is g_saveDirBase followed by g_remixDirSuffix and a `*`, so it enumerates the
     * top-level save directories rather than the files inside one. The mode argument is 0, which
     * starts a fresh listing.
     *
     * @ghidraAddress 0x0017e708
     */
    void ListRemixDir();

    /** @ghidraAddress 0x00186e28 */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Collect the listed directories and read the first one's index.
     *
     * A failed listing abandons the task. Otherwise every entry's name, prefixed with `/`, is
     * appended to mDirNames, and an empty listing reports at once. The first name then moves into
     * mCurrentDir, and a fresh LoadFileMCT reads `<dir>/index` into mBuffer, reporting back through
     * this object's MemcardUser part.
     *
     * @param pOp The finished listing.
     * @ghidraAddress 0x0017e8b8
     */
    virtual void OnListDir(ListDirOp *pOp);

    /**
     * Report the finished listing through MemcardUser::OnRemixesListed().
     *
     * @ghidraAddress 0x00186ea8
     */
    virtual void Finish();

    /**
     * Enquire about the card and start the listing.
     *
     * @ghidraAddress 0x00186df8
     */
    virtual void Execute();

    /**
     * Collect the records of one directory's index and read the next directory's.
     *
     * A failed read abandons the task. Otherwise the index is parsed from mStream and each element
     * appended to mRecords. With no directory left the task reports. Otherwise the next name moves
     * into mCurrentDir, mStream is rewound, and a fresh LoadFileMCT replaces the previous one.
     *
     * @param nStatus The inner read's status.
     * @ghidraAddress 0x0017ece0
     */
    virtual void OnFileLoaded(int nStatus);

private:
    // Selects the step OnListDir() and OnFileLoaded() run next. ListRemixDir() clears it and the
    // constructor does not write it. +0x20
    int mStep;

    // The inner read of the current directory's index. OnListDir() and OnFileLoaded() delete the
    // previous one before creating the next, and the destructor does not release it. +0x24
    LoadFileMCT *mLoadTask;

    // The directory the index is being read out of. +0x28
    HxStr mCurrentDir;

    // Every remix save directory the listing found, consumed one per step. +0x30
    std::vector<HxStr> mDirNames;

    // The collection the parse fills, received as the constructor's last argument. OnFileLoaded()
    // copy-constructs each element through MetRemixRecord's copy constructor at 0x00184130 and
    // steps by 0x38, MetRemixRecord's size. Borrowed, not owned. +0x3c
    std::vector<MetRemixRecord> *mRecords;

    // One index file is read into this stream's buffer at a time. +0x40
    IOBPreallocMemStream mStream;

    // mStream.mBuffer, cached by the constructor. +0x60
    char *mBuffer;
};
