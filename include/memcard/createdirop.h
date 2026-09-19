#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Creation of one directory on a card.
 *
 * `11CreateDirOp` in the RTTI descriptor at `0x008efbb0`, single inheritance from `MemcardOp` at
 * offset 0. An instance is 0x24 bytes and the vtable is at `0x0082bdc0`.
 */
class CreateDirOp : public MemcardOp {
public:
    /**
     * Construct a directory creation.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The directory to create.
     * @param pCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055e628
     */
    CreateDirOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie);

    /** @ghidraAddress 0x0055d738 */
    virtual ~CreateDirOp();

    /** @ghidraAddress 0x0055e6b8 */
    virtual void Issue();

    /** @ghidraAddress 0x0055d7a0 */
    virtual void Complete();

    /**
     * Map `sceMcResNoFormat` to kMemcardStatusNotFormatted, `sceMcResFullDevice` to
     * kMemcardStatusCardFull, and `sceMcResNoEntry` to kMemcardStatusNoEntry.
     *
     * SaveFileMCT proceeds to the open after kMemcardStatusNoEntry as well as after
     * kMemcardStatusOk, and abandons the save only on any other value.
     *
     * @ghidraAddress 0x0055e708
     */
    virtual void InterpretResult();

    /** The directory to create. +0x1c */
    HxStr mPath;
};
