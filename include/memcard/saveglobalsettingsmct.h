#pragma once

#include "memcard/savefilemct.h"
#include "stream/iobpreallocmemstream.h"

class CheckInfoOp;
class GlobalSettings;

/**
 * Save the global settings to a card.
 *
 * `21SaveGlobalSettingsMCT` in the RTTI, single inheritance from `SaveFileMCT` at offset 0. An
 * instance is 0x42c bytes, from MemcardManager::CreateSaveGlobalSettingsTask()'s allocation, and
 * the vtable is at `0x007daa78`.
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
     * @ghidraAddress 0x00178ab0
     */
    SaveGlobalSettingsMCT(
        MemcardUser *pUser, Memcard *pCard, int nPortSlot, int nCookie, GlobalSettings *pSettings);

    /** @ghidraAddress 0x00184e40 */
    virtual ~SaveGlobalSettingsMCT();

    /**
     * Start the save sequence once the card enquiry succeeds.
     *
     * The body is instruction for instruction SavePersonasMCT::OnCheckInfo().
     *
     * @param pOp The finished enquiry.
     * @ghidraAddress 0x00186578
     */
    virtual void OnCheckInfo(CheckInfoOp *pOp);

    /**
     * Report the result through MemcardUser::OnGlobalSettingsSaved().
     *
     * @ghidraAddress 0x00186610
     */
    virtual void Finish();

    /**
     * Aim the save at the settings directory and file, and enquire about the card.
     *
     * @ghidraAddress 0x00178c08
     */
    virtual void Execute();

private:
    // The serialised settings, over g_abRemixStagingBuffer. +0x40c
    IOBPreallocMemStream mStream;
};
