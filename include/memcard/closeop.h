#pragma once

#include "memcard/memcardop.h"

/**
 * Close of an open memory-card file.
 *
 * `7CloseOp` in the RTTI descriptor at `0x008ee460`, single inheritance from `MemcardOp` at offset
 * 0. An instance is 0x20 bytes and the vtable is at `0x0082bc70`.
 *
 * The constructor never writes MemcardOp::mPortSlot, which makes this class and SeekOp the two
 * operations that address a descriptor alone.
 */
class CloseOp : public MemcardOp {
public:
    /**
     * Construct a close against an open descriptor.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nFile The descriptor to close.
     * @param pCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055ee28
     */
    CloseOp(MemcardCBHandler *pHandler, int nFile, void *pCookie);

    /** @ghidraAddress 0x0055df00 */
    virtual ~CloseOp();

    /** @ghidraAddress 0x0055ee50 */
    virtual void Issue();

    /** @ghidraAddress 0x0055df30 */
    virtual void Complete();

    /**
     * Map `sceMcResNoFormat` to kMemcardStatusNotFormatted and `sceMcResNoEntry` to
     * kMemcardStatusBadFile, and report kMemcardStatusOk for zero alone.
     *
     * @ghidraAddress 0x0055ee80
     */
    virtual void InterpretResult();

    /** The descriptor to close. +0x1c */
    int mFile;
};
