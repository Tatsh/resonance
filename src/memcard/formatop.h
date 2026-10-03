#pragma once

#include "memcard/memcardop.h"

/**
 * Format of the card in one slot.
 *
 * Its RTTI descriptor is at `0x008f0270`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x1c bytes, the size of the base alone, and the vtable is at `0x0082be20`.
 */
class FormatOp : public MemcardOp {
public:
    /**
     * Construct a format against one slot.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055e528
     * @ghidraAddress PAL: 0x0059f7f8
     */
    FormatOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d548
     * @ghidraAddress PAL: 0x0059e7a0
     */
    virtual ~FormatOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e550
     * @ghidraAddress PAL: 0x0059f820
     */
    virtual void Issue();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d578
     * @ghidraAddress PAL: 0x0059e7d0
     */
    virtual void Complete();

    /**
     * Report kMemcardStatusOk for zero and kMemcardStatusUnknown for every other result.
     *
     * @ghidraAddress NTSC-U/C: 0x0055e588
     * @ghidraAddress PAL: 0x0059f858
     */
    virtual void InterpretResult();
};
