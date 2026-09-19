#pragma once

#include "memcard/memcardop.h"

/**
 * Format of the card in one slot.
 *
 * `8FormatOp` in the RTTI descriptor at `0x008f0270`, single inheritance from `MemcardOp` at
 * offset 0. An instance is 0x1c bytes, the size of the base alone, and the vtable is at
 * `0x0082be20`.
 */
class FormatOp : public MemcardOp {
public:
    /**
     * Construct a format against one slot.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param pCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055e528
     */
    FormatOp(MemcardCBHandler *pHandler, int nPortSlot, void *pCookie);

    /** @ghidraAddress 0x0055d548 */
    virtual ~FormatOp();

    /** @ghidraAddress 0x0055e550 */
    virtual void Issue();

    /** @ghidraAddress 0x0055d578 */
    virtual void Complete();

    /**
     * Report kMemcardStatusOk for zero and kMemcardStatusUnknown for every other result.
     *
     * @ghidraAddress 0x0055e588
     */
    virtual void InterpretResult();
};
