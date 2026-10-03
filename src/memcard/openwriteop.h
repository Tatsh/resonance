#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Open of one memory-card file for writing, creating it when it is absent.
 *
 * Its RTTI descriptor is at `0x008efb30`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x28 bytes and the vtable is at `0x0082bcd0`.
 *
 * Execute() passes `sceMcFileCreateFile | sceMcFileAttrWriteable` as the open mode, which is the
 * one difference from OpenReadOp.
 */
class OpenWriteOp : public MemcardOp {
public:
    /**
     * Construct an open for writing.
     *
     * @param pHandler The receiver NotifyDone() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The file to open.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055eb58
     * @ghidraAddress PAL: 0x0059fe28
     */
    OpenWriteOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055dca0
     * @ghidraAddress PAL: 0x0059ef18
     */
    virtual ~OpenWriteOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055ebe8
     * @ghidraAddress PAL: 0x0059feb8
     */
    virtual void Execute();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055dd08
     * @ghidraAddress PAL: 0x0059ef90
     */
    virtual void NotifyDone();

    /**
     * Record the descriptor, or map the failure through a jump table.
     *
     * The table at `0x0082bff0` covers the six libmc codes from `sceMcResUpLimitHandle` up to
     * `sceMcResNoFormat`, indexed by the result plus seven.
     *
     * @ghidraAddress NTSC-U/C: 0x0055ec38
     * @ghidraAddress PAL: 0x0059ff08
     */
    virtual void InterpretResult();

    /** The file to open. +0x1c */
    HxStr mPath;

    /** The descriptor, valid once mStatus is kMemcardStatusOk. +0x24 */
    int mFile;
};
