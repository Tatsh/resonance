#pragma once

#include "memcard/memcardop.h"

/**
 * Close of an open memory-card file.
 *
 * Its RTTI descriptor is at `0x008ee460`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x20 bytes and the vtable is at `0x0082bc70`.
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
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055ee28
     * @ghidraAddress PAL: 0x005a00f8
     */
    CloseOp(MemcardCBHandler *pHandler, int nFile, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055df00
     * @ghidraAddress PAL: 0x0059f198
     */
    virtual ~CloseOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055ee50
     * @ghidraAddress PAL: 0x005a0120
     */
    virtual void Issue();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055df30
     * @ghidraAddress PAL: 0x0059f1c8
     */
    virtual void Complete();

    /**
     * Map `sceMcResNoFormat` to kMemcardStatusNotFormatted and `sceMcResNoEntry` to
     * kMemcardStatusBadFile, and report kMemcardStatusOk for zero alone.
     *
     * @ghidraAddress NTSC-U/C: 0x0055ee80
     * @ghidraAddress PAL: 0x005a0150
     */
    virtual void InterpretResult();

    /** The descriptor to close. +0x1c */
    int mFile;
};
