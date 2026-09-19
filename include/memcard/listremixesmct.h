#pragma once

#include <vector>

#include "memcard/loadfilemct.h"
#include "memcard/memcardtask.h"
#include "memcard/memcarduser.h"
#include "os/hxstr.h"
#include "stream/iobpreallocmemstream.h"

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
 * Three routines are recovered and not written. `OnListDir()` at `0x0017e8b8` collects the listed
 * directory names into mDirNames, and `OnFileLoaded()` at `0x0017ece0` parses one index and steps
 * to the next directory. Both walk the record the string `********** RemixIndex element **********`
 * at `0x007d2b18` titles, whose six fields are `LevelName`, `RemixName`, `FileName`, `GameOK`,
 * `Version`, and `AlbumNum`, and that record's class cannot be titled from the image. The
 * constructor at `0x0017e528` is not written for the same reason, because its last argument is a
 * pointer to the collection the parse fills.
 *
 * The method titles ListRemixDir(), Execute(), and Finish() are inferred. No string in the image
 * identifies any of them.
 */
class ListRemixesMCT : public MemcardTask, public MemcardUser {
public:
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

    /** @ghidraAddress 0x0017ece0 */
    virtual void OnFileLoaded(int nStatus);

private:
    // Selects the step OnListDir() and OnFileLoaded() run next. ListRemixDir() clears it and the
    // constructor does not write it. +0x20
    int mStep;

    // +0x24
    int mUnknown24;

    // The directory the index is being read out of. +0x28
    HxStr mCurrentDir;

    // Every remix save directory the listing found, consumed one per step. +0x30
    std::vector<HxStr> mDirNames;

    // Addresses the collection the parse fills, whose element class cannot be titled. The
    // constructor receives it as its last argument. Recorded as a reserved word rather than as a
    // pointer, because the type it points at is not recovered. +0x3c
    unsigned char mReserved3c[4];

    // One index file is read into this stream's buffer at a time. +0x40
    IOBPreallocMemStream mStream;

    // mStream.mBuffer, cached by the constructor. +0x60
    char *mBuffer;
};
