#pragma once

#include "memcard/savefilemct.h"
#include "stream/iobpreallocmemstream.h"

class CheckInfoOp;
class GlobalSettings;

/**
 * Save the global settings to a card.
 *
 * It has single inheritance from `SaveFileMCT` at offset 0. An instance is 0x42c bytes, from
 * MemcardManager::CreateSaveGlobalSettingsTask()'s allocation, and the vtable is at `0x007daa78`.
 *
 * The constructor serialises the settings into g_abRemixStagingBuffer through mStream at once.
 * Execute() then points the inherited save sequence at that buffer and starts it with a card
 * enquiry.
 */
class SaveGlobalSettingsMCT : public SaveFileMCT {
public:
    /**
     * Construct an idle save and serialise the settings through GlobalSettings::Save().
     *
     * @param pUser The receiver Finish() reports to.
     * @param pCard The queue the task submits operations to.
     * @param nPortSlot The packed port and slot.
     * @param nCookie The tag that abandons exactly this task's operations.
     * @param pSettings The settings to save.
     * @ghidraAddress NTSC-U/C: 0x00178ab0
     * @ghidraAddress PAL: 0x0017c348
     */
    SaveGlobalSettingsMCT(
        MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, GlobalSettings *pSettings);

    /**
     * @ghidraAddress NTSC-U/C: 0x00184e40
     * @ghidraAddress PAL: 0x0018a468
     */
    virtual ~SaveGlobalSettingsMCT();

#ifndef VIDEO_STANDARD_PAL
    /**
     * Start the save sequence once the card enquiry succeeds.
     *
     * The body is instruction for instruction SavePersonasMCT::OnCheckInfo(). The European release
     * inherits SaveFileMCT::OnCheckInfo() instead.
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress NTSC-U/C: 0x00186578
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);
#endif

    /**
     * Report the result through MemcardUser::OnGlobalSettingsSaved().
     *
     * @ghidraAddress NTSC-U/C: 0x00186610
     * @ghidraAddress PAL: 0x0018bfa0
     */
    virtual void Finish();

    /**
     * Aim the save at the settings directory and file, and enquire about the card.
     *
     * The European release adds the settings to SaveFileMCT::mFiles and runs
     * SaveFileMCT::Execute().
     *
     * @ghidraAddress NTSC-U/C: 0x00178c08
     * @ghidraAddress PAL: 0x0017c590
     */
    virtual void Execute();

private:
    // The serialised settings, over g_abRemixStagingBuffer. +0x40c in the North American release
    // and +0x41c in the European release.
    IOBPreallocMemStream mStream;
};
