#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Open of one memory-card file for reading.
 *
 * Its RTTI descriptor is at `0x008efb40`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x28 bytes and the vtable is at `0x0082bca0`.
 *
 * Execute() passes `sceMcFileAttrReadable` as the open mode, which is the one difference from
 * OpenWriteOp.
 */
class OpenReadOp : public MemcardOp {
public:
    /**
     * Construct an open for reading.
     *
     * @param pHandler The receiver NotifyDone() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The file to open.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055ecc0
     * @ghidraAddress PAL: 0x0059ff90
     */
    OpenReadOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055ddd0
     * @ghidraAddress PAL: 0x0059f058
     */
    virtual ~OpenReadOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055ed50
     * @ghidraAddress PAL: 0x005a0020
     */
    virtual void Execute();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055de38
     * @ghidraAddress PAL: 0x0059f0d0
     */
    virtual void NotifyDone();

    /**
     * Record the descriptor, or map the failure.
     *
     * The body writes mFile from the result before it tests the sign. A failed open therefore
     * stores the libmc error code in mFile as well as in MemcardOp::mResult. The behaviour matches
     * the binary.
     *
     * @ghidraAddress NTSC-U/C: 0x0055eda0
     * @ghidraAddress PAL: 0x005a0070
     */
    virtual void InterpretResult();

    /** The file to open. +0x1c */
    HxStr mPath;

    /** The descriptor, valid once mStatus is kMemcardStatusOk. +0x24 */
    int mFile;
};
