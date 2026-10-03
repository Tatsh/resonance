#pragma once

#include "memcard/memcardop.h"

/**
 * Write to an open memory-card file.
 *
 * Its RTTI descriptor is at `0x008f4840`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x2c bytes and the vtable is at `0x0082bd30`.
 *
 * Execute() addresses the descriptor rather than the card. MemcardOp::mPortSlot is therefore
 * recorded and never read.
 */
class WriteOp : public MemcardOp {
public:
    /**
     * Construct a write against an open descriptor.
     *
     * @param pHandler The receiver NotifyDone() reports to.
     * @param nPortSlot The packed port and slot, which Execute() does not use.
     * @param nFile The descriptor OpenWriteOp delivered.
     * @param pBuffer The source.
     * @param nLength The number of bytes to write.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055e9b8
     * @ghidraAddress PAL: 0x0059fc88
     */
    WriteOp(MemcardCBHandler *pHandler,
            int nPortSlot,
            int nFile,
            const void *pBuffer,
            int nLength,
            int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055dab0
     * @ghidraAddress PAL: 0x0059ed28
     */
    virtual ~WriteOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e9e8
     * @ghidraAddress PAL: 0x0059fcb8
     */
    virtual void Execute();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055dae0
     * @ghidraAddress PAL: 0x0059ed58
     */
    virtual void NotifyDone();

    /**
     * Record the transferred byte count, or map the failure through a jump table.
     *
     * The table at `0x0082bfd0` covers the seven libmc codes from `sceMcResFailReplace` up to
     * `sceMcResNoFormat`, indexed by the result plus eight. This is the only operation that
     * produces kMemcardStatusWriteDenied.
     *
     * @ghidraAddress NTSC-U/C: 0x0055ea20
     * @ghidraAddress PAL: 0x0059fcf0
     */
    virtual void InterpretResult();

    /** Bytes transferred, valid once mStatus is kMemcardStatusOk. +0x1c */
    int mBytesTransferred;

    /** The descriptor to write to. +0x20 */
    int mFile;

    /** The source. +0x24 */
    const void *mBuffer;

    /** The number of bytes to write. +0x28 */
    int mLength;
};
