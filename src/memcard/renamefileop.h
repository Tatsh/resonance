#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Rename of one file or directory on a card.
 *
 * Its RTTI descriptor is at `0x008f4990`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x2c bytes and the vtable is at `0x0082bc10`.
 *
 * `Memcard::RenameFile()` has no caller in the image. Nothing exercises the class.
 */
class RenameFileOp : public MemcardOp {
public:
    /**
     * Construct a rename.
     *
     * @param pHandler The receiver NotifyDone() reports to.
     * @param nPortSlot The packed port and slot.
     * @param oldPath The existing name.
     * @param newPath The replacement name.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055f030
     * @ghidraAddress PAL: 0x005a0300
     */
    RenameFileOp(MemcardCBHandler *pHandler,
                 int nPortSlot,
                 const HxStr &oldPath,
                 const HxStr &newPath,
                 int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e128
     * @ghidraAddress PAL: 0x0059f3d0
     */
    virtual ~RenameFileOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055f108
     * @ghidraAddress PAL: 0x005a03e8
     */
    virtual void Execute();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e1a0
     * @ghidraAddress PAL: 0x0059f470
     */
    virtual void NotifyDone();

    /**
     * Map `sceMcResNoFormat` to kMemcardStatusNotFormatted, `sceMcResFullDevice` to
     * kMemcardStatusCardFull, and `sceMcResNoEntry` to kMemcardStatusNoEntry.
     *
     * @ghidraAddress NTSC-U/C: 0x0055f168
     * @ghidraAddress PAL: 0x005a0448
     */
    virtual void InterpretResult();

    /** The existing name. +0x1c */
    HxStr mOldPath;

    /** The replacement name. +0x24 */
    HxStr mNewPath;
};
