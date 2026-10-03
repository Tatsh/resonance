#pragma once

#include "memcard/memcardop.h"

/**
 * Unformat of the card in one slot.
 *
 * Its RTTI descriptor is at `0x008efa60`. It has single inheritance from `MemcardOp` at offset 0.
 * An instance is 0x1c bytes, the size of the base alone, and the vtable is at `0x0082bdf0`.
 *
 * FormatCardMCT queues an unformat immediately before a format, which is how the game clears a
 * card that reports itself as formatted but unusable.
 */
class UnformatOp : public MemcardOp {
public:
    /**
     * Construct an unformat against one slot.
     *
     * @param pHandler The receiver NotifyDone() reports to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag Memcard::Cancel() matches on.
     * @ghidraAddress NTSC-U/C: 0x0055e5a8
     * @ghidraAddress PAL: 0x0059f878
     */
    UnformatOp(MemcardCBHandler *pHandler, int nPortSlot, int nCookie);

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d640
     * @ghidraAddress PAL: 0x0059e898
     */
    virtual ~UnformatOp();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055e5d0
     * @ghidraAddress PAL: 0x0059f8a0
     */
    virtual void Execute();

    /**
     * @ghidraAddress NTSC-U/C: 0x0055d670
     * @ghidraAddress PAL: 0x0059e8c8
     */
    virtual void NotifyDone();

    /**
     * Report kMemcardStatusOk for zero and kMemcardStatusUnknown for every other result.
     *
     * @ghidraAddress NTSC-U/C: 0x0055e608
     * @ghidraAddress PAL: 0x0059f8d8
     */
    virtual void InterpretResult();
};
