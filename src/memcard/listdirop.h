#pragma once

#include <libmc.h>

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/** Entries one listing delivers, which is the `maxent` argument Execute() passes to libmc. */
constexpr int kListDirMaxEntries = 20;

/**
 * Listing of one directory on a card.
 *
 * Its RTTI descriptor is at `0x008f47c0`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x34 bytes and the vtable is at `0x0082bd90`.
 *
 * Every listing writes into the one shared table at `0x00726a40`. A second listing therefore
 * overwrites the first, and mEntries addresses that table rather than storage of its own.
 */
class ListDirOp : public MemcardOp {
public:
    /**
     * Construct a listing.
     *
     * @param pHandler The receiver NotifyDone() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The directory to list.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @param nMode The `sceMcGetDir()` mode, which selects between a fresh listing and a
     *              continuation of the previous one.
     * @ghidraAddress NTSC-U/C: 0x0055e778
     * @ghidraAddress PAL: 0x0059fa48
     */
    ListDirOp(
        MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie, unsigned nMode);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d868
     * @ghidraAddress PAL: 0x0059ead0
     */
    virtual ~ListDirOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e818
     * @ghidraAddress PAL: 0x0059fae8
     */
    virtual void Execute();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d8d0
     * @ghidraAddress PAL: 0x0059eb48
     */
    virtual void NotifyDone();

    /**
     * Record the entry count and publish the shared table, or map the failure.
     *
     * `sceMcResNoEntry` becomes kMemcardStatusNoDirectory here rather than kMemcardStatusNoEntry,
     * which is the one place that value is produced.
     *
     * @ghidraAddress NTSC-U/C: 0x0055e870
     * @ghidraAddress PAL: 0x0059fb40
     */
    virtual void InterpretResult();

    /** The directory the listing covers. +0x1c */
    HxStr mPath;

    /** Entries the listing delivered, valid once mStatus is kMemcardStatusOk. +0x24 */
    int mEntryCount;

    /** The `sceMcGetDir()` mode. +0x28 */
    unsigned mMode;

    /** Non-zero once the listing filled the table and further entries may remain. +0x2c */
    int mTruncated;

    /** The shared entry table at `0x00726a40`. +0x30 */
    sceMcTblGetDir *mEntries;
};

/**
 * Table every listing writes into.
 *
 * @ghidraAddress NTSC-U/C: 0x00726a40
 * @ghidraAddress PAL: 0x0076a700
 */
extern sceMcTblGetDir g_aMemcardDirEntries[kListDirMaxEntries];
