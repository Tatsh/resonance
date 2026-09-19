#pragma once

#include "memcard/memcardop.h"

/**
 * Read from an open memory-card file.
 *
 * `6ReadOp` in the RTTI descriptor at `0x008ee7c8`, single inheritance from `MemcardOp` at offset
 * 0. An instance is 0x2c bytes and the vtable is at `0x0082bd60`.
 *
 * Issue() addresses the descriptor rather than the card. MemcardOp::mPortSlot is therefore
 * recorded and never read.
 */
class ReadOp : public MemcardOp {
public:
    /**
     * Construct a read against an open descriptor.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot, which Issue() does not use.
     * @param nFile The descriptor OpenReadOp delivered.
     * @param pBuffer The destination.
     * @param nLength The number of bytes to read.
     * @param pCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055e8e0
     */
    ReadOp(MemcardCBHandler *pHandler,
           int nPortSlot,
           int nFile,
           void *pBuffer,
           int nLength,
           void *pCookie);

    /** @ghidraAddress 0x0055d9b8 */
    virtual ~ReadOp();

    /** @ghidraAddress 0x0055e910 */
    virtual void Issue();

    /** @ghidraAddress 0x0055d9e8 */
    virtual void Complete();

    /**
     * Record the transferred byte count, or map the failure.
     *
     * `sceMcResNoEntry` becomes kMemcardStatusBadFile and `sceMcResDeniedPermit` becomes
     * kMemcardStatusNoEntry, which is the reverse of the pairing the open operations use.
     *
     * @ghidraAddress 0x0055e948
     */
    virtual void InterpretResult();

    /** Bytes transferred, valid once mStatus is kMemcardStatusOk. +0x1c */
    int mBytesTransferred;

    /** The descriptor to read from. +0x20 */
    int mFile;

    /** The destination. +0x24 */
    void *mBuffer;

    /** The number of bytes to read. +0x28 */
    int mLength;
};
