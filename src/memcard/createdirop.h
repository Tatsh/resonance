#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Creation of one directory on a card.
 *
 * Its RTTI descriptor is at `0x008efbb0`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x24 bytes and the vtable is at `0x0082bdc0`.
 */
class CreateDirOp : public MemcardOp {
public:
    /**
     * Construct a directory creation.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The directory to create.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055e628
     * @ghidraAddress PAL: 0x0059f8f8
     */
    CreateDirOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d738
     * @ghidraAddress PAL: 0x0059e990
     */
    virtual ~CreateDirOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e6b8
     * @ghidraAddress PAL: 0x0059f988
     */
    virtual void Issue();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d7a0
     * @ghidraAddress PAL: 0x0059ea08
     */
    virtual void Complete();

    /**
     * Map `sceMcResNoFormat` to kMemcardStatusNotFormatted, `sceMcResFullDevice` to
     * kMemcardStatusCardFull, and `sceMcResNoEntry` to kMemcardStatusNoEntry.
     *
     * SaveFileMCT proceeds to the open after kMemcardStatusNoEntry as well as after
     * kMemcardStatusOk, and abandons the save only on any other value.
     *
     * @ghidraAddress NTSC-U/C: 0x0055e708
     * @ghidraAddress PAL: 0x0059f9d8
     */
    virtual void InterpretResult();

    /** The directory to create. +0x1c */
    HxStr mPath;
};
