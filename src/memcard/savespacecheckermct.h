#pragma once

#include <vector>

#include "memcard/memcardtask.h"
#include "memcard/savefilemct.h"
#include "os/hxstr.h"

/**
 * Measure the kilobytes a SaveFileMCT's files need beyond the space they occupy on the card now.
 *
 * The European release added the class, and the North American release has no counterpart. Its
 * RTTI type name is `SaveSpaceCheckerMCT`. It has single inheritance from `MemcardTask` at offset
 * 0. An instance is 0x2c bytes, from the allocation in SaveFileMCT::StartSpaceCheck(), and the
 * vtable is at `0x0081ed00`.
 *
 * SaveFileMCT constructs the task directly instead of queueing it through MemcardManager. The task
 * has no MemcardUser, and Execute() never runs. The constructor lists the save directory at once,
 * and OnListDir() reports the measurement directly to the SaveFileMCT that started it, through
 * SaveFileMCT::OnSpaceChecked(). A file already in the directory costs the kilobytes it grows by, a
 * missing file costs its full size, and a missing directory costs two kilobytes more. Each pair of
 * new files, rounding up, costs one kilobyte of directory entries. A kilobyte here is one card
 * cluster.
 *
 * The owner deletes the task when it creates the directory.
 */
class SaveSpaceCheckerMCT : public MemcardTask {
public:
    /**
     * Construct the check and list the save directory.
     *
     * The listing covers every entry of dirName, through the wildcard `*`.
     *
     * @param pOwner The save the measurement is for.
     * @param pFiles The files the save writes. The vector stays in pOwner and is not copied.
     * @param dirName The save directory. The task stores a copy.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @ghidraAddress PAL: 0x0017ae70
     */
    SaveSpaceCheckerMCT(SaveFileMCT *pOwner,
                        const std::vector<SaveFileEntry> *pFiles,
                        const HxStr &dirName,
                        Memcard *pCard,
                        int nPortSlot,
                        int nCookie);

    /** @ghidraAddress PAL: 0x00189ac0 */
    virtual ~SaveSpaceCheckerMCT();

    /**
     * Measure the files against the listing and report to the owner.
     *
     * A failed listing other than kMemcardStatusNoDirectory abandons the task's operations and
     * still reports to the owner, with a measurement of zero.
     *
     * @param pOp The finished listing.
     * @ghidraAddress PAL: 0x0017b050
     */
    virtual void OnListDir(ListDirOp *pOp);

    /**
     * Do nothing.
     *
     * @ghidraAddress PAL: 0x00189828
     */
    virtual void Finish();

    /**
     * Write a diagnostic with the routine's name to `cout`.
     *
     * The routine is never called.
     *
     * @ghidraAddress PAL: 0x00189830
     */
    virtual void Execute();

private:
    // The save the measurement is for. +0x1c
    SaveFileMCT *mOwner;

    // The files the save writes. +0x20
    const std::vector<SaveFileEntry> *mFiles;

    // The save directory. +0x24
    HxStr mDirName;
};
