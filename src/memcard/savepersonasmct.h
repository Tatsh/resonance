#pragma once

#include <vector>

#include "memcard/savefilemct.h"
#include "stream/iobpreallocmemstream.h"

class CheckInfoOp;
class MetPersonaData;

/**
 * Save the FreQ roster to a card.
 *
 * It has single inheritance from `SaveFileMCT` at offset 0. An instance is 0x42c bytes, from
 * MemcardManager::CreateSavePersonasTask()'s allocation, and the vtable is at `0x007dab18`. In the
 * European release an instance is 0x43c bytes and the vtable is at `0x0081e970`.
 *
 * The constructor serialises the roster into g_abRemixStagingBuffer through mStream at once. The
 * save therefore writes the roster as it stood when the task was queued. Execute() then points the
 * inherited save sequence at that buffer and starts it with a card enquiry.
 */
class SavePersonasMCT : public SaveFileMCT {
public:
    /**
     * Construct an idle save and serialise the roster.
     *
     * Writes the persona count and then each persona through MetPersonaData::Save().
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param roster The personas to save.
     * @ghidraAddress NTSC-U/C: 0x00178798
     * @ghidraAddress PAL: 0x0017be58
     */
    SavePersonasMCT(MemcardUser *pUser,
                    Memcard *pCard,
                    int nPortSlot,
                    int nCookie,
                    const std::vector<MetPersonaData *> &roster);

    /**
     * @ghidraAddress NTSC-U/C: 0x00184d98
     * @ghidraAddress PAL: 0x0018a2e8
     */
    virtual ~SavePersonasMCT();

#ifndef VIDEO_STANDARD_PAL
    /**
     * Start the save sequence once the card enquiry succeeds.
     *
     * A status of kMemcardStatusUnknown or kMemcardStatusNotFormatted abandons the task with that
     * status, and fewer than kSaveFileMinimumFreeClusters free clusters abandon it with
     * kMemcardStatusCardFull. Any other status starts the sequence from its first step. The
     * European release inherits SaveFileMCT::OnCheckInfo() instead.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress NTSC-U/C: 0x001864a0
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);
#endif

    /**
     * Report the result through MemcardUser::OnPersonasSaved().
     *
     * @ghidraAddress NTSC-U/C: 0x00186538
     * @ghidraAddress PAL: 0x0018bf60
     */
    virtual void Finish();

    /**
     * Aim the save at the roster directory and file, and enquire about the card.
     *
     * The European release adds the roster to SaveFileMCT::mFiles and runs SaveFileMCT::Execute().
     *
     * @ghidraAddress NTSC-U/C: 0x00178960
     * @ghidraAddress PAL: 0x0017c110
     */
    virtual void Execute();

private:
    // The serialised roster, over g_abRemixStagingBuffer. +0x40c in the North American release and
    // +0x41c in the European release.
    IOBPreallocMemStream mStream;
};
