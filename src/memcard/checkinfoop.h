#pragma once

#include "memcard/memcardop.h"

/**
 * Enquiry about the card in one slot.
 *
 * Its RTTI descriptor is at `0x008efba0`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x28 bytes and the vtable is at `0x0082be80`.
 *
 * Issue() calls `sceMcGetInfo()`, which fills all three result members at once.
 */
class CheckInfoOp : public MemcardOp {
public:
    /**
     * Construct an enquiry against one slot.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055e368
     * @ghidraAddress PAL: 0x0059f638
     */
    CheckInfoOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d320
     * @ghidraAddress PAL: 0x0059e568
     */
    virtual ~CheckInfoOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e390
     * @ghidraAddress PAL: 0x0059f660
     */
    virtual void Issue();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d350
     * @ghidraAddress PAL: 0x0059e598
     */
    virtual void Complete();

    /**
     * Report kMemcardStatusNotFormatted for `sceMcResNoFormat`, and kMemcardStatusOk for both zero
     * and `sceMcResChangedCard`.
     *
     * A changed card is therefore not an error here, and a positive result is.
     *
     * @ghidraAddress NTSC-U/C: 0x0055e3d8
     * @ghidraAddress PAL: 0x0059f6a8
     */
    virtual void InterpretResult();

    /** One of `sceMcTypeNoCard`, `sceMcTypePS1`, `sceMcTypePS2` or `sceMcTypePDA`. +0x1c */
    int mType;

    /** Non-zero once the card is formatted. +0x20 */
    int mFormatted;

    /** Free clusters on the card. +0x24 */
    int mFree;
};
