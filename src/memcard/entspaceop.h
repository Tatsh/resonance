#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Enquiry about the free directory entries under one path.
 *
 * Its RTTI descriptor is at `0x008ef048`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x28 bytes and the vtable is at `0x0082be50`.
 *
 * Issue() calls `sceMcGetEntSpace()`, whose result is the free entry count rather than a status, so
 * InterpretResult() treats every value that is not negative as success. The count survives only in
 * MemcardOp::mResult, because InterpretResult() copies it nowhere.
 */
class EntSpaceOp : public MemcardOp {
public:
    /**
     * Construct an enquiry against one directory.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param path The directory to measure.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055e418
     * @ghidraAddress PAL: 0x0059f6e8
     */
    EntSpaceOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d418
     * @ghidraAddress PAL: 0x0059e660
     */
    virtual ~EntSpaceOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e4a8
     * @ghidraAddress PAL: 0x0059f778
     */
    virtual void Issue();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d480
     * @ghidraAddress PAL: 0x0059e6d8
     */
    virtual void Complete();

    /**
     * Report kMemcardStatusOk for any result that is not negative, and
     * kMemcardStatusNotFormatted for `sceMcResNoFormat`.
     *
     * @ghidraAddress NTSC-U/C: 0x0055e4f8
     * @ghidraAddress PAL: 0x0059f7c8
     */
    virtual void InterpretResult();

    /** The directory the enquiry measures. +0x1c */
    HxStr mPath;

    /**
     * Reserved.
     *
     * The operation is 0x28 bytes where its recovered members end at 0x24, and the word is
     * neither written by the constructor nor read anywhere in the image. `Memcard::EntSpace()` is
     * itself never called. The class is never exercised.
     *
     * +0x24
     */
    int mReserved24;
};
