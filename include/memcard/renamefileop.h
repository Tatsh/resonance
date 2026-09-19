#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Rename of one file or directory on a card.
 *
 * `12RenameFileOp` in the RTTI descriptor at `0x008f4990`, single inheritance from `MemcardOp` at
 * offset 0. An instance is 0x2c bytes and the vtable is at `0x0082bc10`.
 *
 * `Memcard::RenameFile()` has no caller in the image, so nothing exercises the class.
 */
class RenameFileOp : public MemcardOp {
public:
    /**
     * Construct a rename.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param oldPath The existing name.
     * @param newPath The replacement name.
     * @param pCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055f030
     */
    RenameFileOp(MemcardCBHandler *pHandler,
                 int nPortSlot,
                 const HxStr &oldPath,
                 const HxStr &newPath,
                 void *pCookie);

    /** @ghidraAddress 0x0055e128 */
    virtual ~RenameFileOp();

    /** @ghidraAddress 0x0055f108 */
    virtual void Issue();

    /** @ghidraAddress 0x0055e1a0 */
    virtual void Complete();

    /**
     * Map `sceMcResNoFormat` to kMemcardStatusNotFormatted, `sceMcResFullDevice` to
     * kMemcardStatusCardFull, and `sceMcResNoEntry` to kMemcardStatusNoEntry.
     *
     * @ghidraAddress 0x0055f168
     */
    virtual void InterpretResult();

    /** The existing name. +0x1c */
    HxStr mOldPath;

    /** The replacement name. +0x24 */
    HxStr mNewPath;
};
