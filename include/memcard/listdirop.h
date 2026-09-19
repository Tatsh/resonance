#pragma once

#include <libmc.h>

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/** Entries one listing delivers, which is the `maxent` argument Issue() passes to libmc. */
constexpr int kListDirMaxEntries = 20;

/**
 * Listing of one directory on a card.
 *
 * `9ListDirOp` in the RTTI descriptor at `0x008f47c0`, single inheritance from `MemcardOp` at
 * offset 0. An instance is 0x34 bytes and the vtable is at `0x0082bd90`.
 *
 * Every listing writes into the one shared table at `0x00726a40`, so a second listing overwrites
 * the first, and mEntries addresses that table rather than storage of its own.
 */
class ListDirOp : public MemcardOp {
public:
    /**
     * Construct a listing.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The directory to list, which libmc accepts with wildcards.
     * @param pCookie The tag Memcard::Cancel() matches on.
     * @param nMode The `sceMcGetDir()` mode, which selects between a fresh listing and a
     *              continuation of the previous one.
     * @ghidraAddress 0x0055e778
     */
    ListDirOp(MemcardCBHandler *pHandler,
              int nPortSlot,
              const HxStr &path,
              void *pCookie,
              unsigned nMode);

    /** @ghidraAddress 0x0055d868 */
    virtual ~ListDirOp();

    /** @ghidraAddress 0x0055e818 */
    virtual void Issue();

    /** @ghidraAddress 0x0055d8d0 */
    virtual void Complete();

    /**
     * Record the entry count and publish the shared table, or map the failure.
     *
     * `sceMcResNoEntry` becomes kMemcardStatusNoDirectory here rather than kMemcardStatusNoEntry,
     * which is the one place that value is produced.
     *
     * @ghidraAddress 0x0055e870
     */
    virtual void InterpretResult();

    /** The directory the listing covers. +0x1c */
    HxStr mPath;

    /** Entries the listing delivered, valid once mStatus is kMemcardStatusOk. +0x24 */
    int mEntryCount;

    /** The `sceMcGetDir()` mode. +0x28 */
    unsigned mMode;

    /** Non-zero once the listing filled the table, so that further entries may remain. +0x2c */
    int mTruncated;

    /** The shared entry table at `0x00726a40`. +0x30 */
    sceMcTblGetDir *mEntries;
};

/**
 * Table every listing writes into.
 *
 * @ghidraAddress 0x00726a40
 */
extern sceMcTblGetDir g_aMemcardDirEntries[kListDirMaxEntries];
