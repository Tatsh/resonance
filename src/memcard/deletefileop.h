#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Deletion of one file or directory on a card.
 *
 * Its RTTI descriptor is at `0x008efbe0`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x24 bytes and the vtable is at `0x0082bc40`.
 */
class DeleteFileOp : public MemcardOp {
public:
    /**
     * Construct a deletion.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The file or directory to delete.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055eed8
     * @ghidraAddress PAL: 0x005a01a8
     */
    DeleteFileOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055dff8
     * @ghidraAddress PAL: 0x0059f290
     */
    virtual ~DeleteFileOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055ef68
     * @ghidraAddress PAL: 0x005a0238
     */
    virtual void Issue();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e060
     * @ghidraAddress PAL: 0x0059f308
     */
    virtual void Complete();

    /**
     * Map the result through a jump table.
     *
     * The table at `0x0082c010` covers the seven libmc codes from `sceMcResNotEmpty` up to zero,
     * indexed by the result plus six. A positive result falls outside the table and reports
     * kMemcardStatusUnknown. This is the only operation that produces kMemcardStatusNoFile.
     *
     * @ghidraAddress NTSC-U/C: 0x0055efb8
     * @ghidraAddress PAL: 0x005a0288
     */
    virtual void InterpretResult();

    /** The file or directory to delete. +0x1c */
    HxStr mPath;
};
