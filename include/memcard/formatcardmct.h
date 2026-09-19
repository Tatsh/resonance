#pragma once

#include "memcard/memcardtask.h"

/**
 * Status FormatCardMCT reports when the card is already formatted.
 *
 * The value is produced by this task alone and by no MemcardOp, which is why it is absent from
 * MemcardStatus. Nothing in the image identifies it further than the branch that writes it.
 */
constexpr int kMemcardStatusAlreadyFormatted = 13;

/**
 * Format or unformat the card in one slot.
 *
 * `13FormatCardMCT` in the RTTI descriptor at `0x008ef290`, single inheritance from `MemcardTask`
 * at offset 0. An instance is 0x20 bytes and the vtable is at `0x007dabb8`. The accessor at
 * `0x00184c90` belongs to this class rather than to `MemcardTask`, which the descriptor it guards
 * on at entry establishes, and `rtti.json` attributes to `MemcardTask` in error.
 *
 * The task enquires first and refuses to act on a card that reports itself formatted, which is
 * what kMemcardStatusAlreadyFormatted is for. Otherwise it queues a format, or an unformat when
 * mUnformat is set, and reports through whichever MemcardUser method matches the direction.
 *
 * One asymmetry is faithful and looks like an oversight in the original. The class overrides
 * `OnFormat()` but not `OnUnformat()`, so an unformat's completion arrives at the empty default
 * body of `MemcardCBHandler` and the task never finishes. Only the format direction is observed.
 */
class FormatCardMCT : public MemcardTask {
public:
    /**
     * Construct an idle format task.
     *
     * The constructor is inlined at every site and no address of its own survives. mUnformat is
     * the one member, and the argument that writes it is not established, because no construction
     * site has been located.
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param pCookie The tag that abandons exactly this task's operations.
     */
    FormatCardMCT(MemcardUser *pUser, Memcard *pCard, int nPortSlot, void *pCookie);

    /** @ghidraAddress 0x00184d68 */
    virtual ~FormatCardMCT();

    /**
     * Queue the format or the unformat, whichever mUnformat selects.
     *
     * A second copy of this dispatch survives at `0x00186348` with no caller, which is one of this
     * compiler's unfolded per-unit emissions.
     *
     * @ghidraAddress 0x00186348
     */
    void IssueFormat();

    /**
     * Refuse a formatted card, and otherwise queue the format.
     *
     * A card reporting itself formatted yields kMemcardStatusAlreadyFormatted and the task
     * abandons its work. kMemcardStatusUnknown abandons it as well. Any other status proceeds.
     *
     * @ghidraAddress 0x00186390
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /** @ghidraAddress 0x00186438 */
    virtual void OnFormat(FormatOp *pOp);

    /** @ghidraAddress 0x001862b0 */
    virtual void Finish();

    /** @ghidraAddress 0x00186318 */
    virtual void Execute();

private:
    // Zero to format, non-zero to unformat. Also selects which MemcardUser method Finish() reports
    // to. +0x1c
    int mUnformat;
};
