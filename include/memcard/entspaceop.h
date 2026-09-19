#pragma once

#include "memcard/memcardop.h"
#include "os/hxstr.h"

/**
 * Enquiry about the free directory entries under one path.
 *
 * `10EntSpaceOp` in the RTTI descriptor at `0x008ef048`, single inheritance from `MemcardOp` at
 * offset 0. An instance is 0x28 bytes and the vtable is at `0x0082be50`.
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
     * @param pCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055e418
     */
    EntSpaceOp(MemcardCBHandler *pHandler, int nPortSlot, const HxStr &path, void *pCookie);

    /** @ghidraAddress 0x0055d418 */
    virtual ~EntSpaceOp();

    /** @ghidraAddress 0x0055e4a8 */
    virtual void Issue();

    /** @ghidraAddress 0x0055d480 */
    virtual void Complete();

    /**
     * Report kMemcardStatusOk for any result that is not negative, and
     * kMemcardStatusNotFormatted for `sceMcResNoFormat`.
     *
     * @ghidraAddress 0x0055e4f8
     */
    virtual void InterpretResult();

    /** The directory the enquiry measures. +0x1c */
    HxStr mPath;

    /**
     * Unrecovered.
     *
     * The operation is 0x28 bytes where its recovered members end at 0x24, and the word is
     * neither written by the constructor nor read anywhere in the image. `Memcard::EntSpace()` is
     * itself never called. Nothing exercises the class.
     *
     * +0x24
     */
    int mUnknown24;
};
