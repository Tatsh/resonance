#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Open of one memory-card file for writing, creating it when it is absent.
 *
 * `11OpenWriteOp` in the RTTI descriptor at `0x008efb30`, single inheritance from `MemcardOp` at
 * offset 0. An instance is 0x28 bytes and the vtable is at `0x0082bcd0`.
 *
 * Issue() passes `sceMcFileCreateFile | sceMcFileAttrWriteable` as the open mode, which is the one
 * difference from OpenReadOp.
 */
class OpenWriteOp : public MemcardOp {
public:
    /**
     * Construct an open for writing.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The file to open.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055eb58
     */
    OpenWriteOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie);

    /** @ghidraAddress 0x0055dca0 */
    virtual ~OpenWriteOp();

    /** @ghidraAddress 0x0055ebe8 */
    virtual void Issue();

    /** @ghidraAddress 0x0055dd08 */
    virtual void Complete();

    /**
     * Record the descriptor, or map the failure through a jump table.
     *
     * The table at `0x0082bff0` covers the six libmc codes from `sceMcResUpLimitHandle` up to
     * `sceMcResNoFormat`, indexed by the result plus seven.
     *
     * @ghidraAddress 0x0055ec38
     */
    virtual void InterpretResult();

    /** The file to open. +0x1c */
    HxStr mPath;

    /** The descriptor, valid once mStatus is kMemcardStatusOk. +0x24 */
    int mFile;
};
