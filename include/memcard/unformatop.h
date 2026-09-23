#pragma once

#include "memcard/memcardop.h"

/**
 * Unformat of the card in one slot.
 *
 * `10UnformatOp` in the RTTI descriptor at `0x008efa60`, single inheritance from `MemcardOp` at
 * offset 0. An instance is 0x1c bytes, the size of the base alone, and the vtable is at
 * `0x0082bdf0`.
 *
 * FormatCardMCT queues an unformat immediately before a format, which is how the game clears a
 * card that reports itself as formatted but unusable.
 */
class UnformatOp : public MemcardOp {
public:
    /**
     * Construct an unformat against one slot.
     *
     * @param pHandler The receiver Complete() reports to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress 0x0055e5a8
     */
    UnformatOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie);

    /** @ghidraAddress 0x0055d640 */
    virtual ~UnformatOp();

    /** @ghidraAddress 0x0055e5d0 */
    virtual void Issue();

    /** @ghidraAddress 0x0055d670 */
    virtual void Complete();

    /**
     * Report kMemcardStatusOk for zero and kMemcardStatusUnknown for every other result.
     *
     * @ghidraAddress 0x0055e608
     */
    virtual void InterpretResult();
};
